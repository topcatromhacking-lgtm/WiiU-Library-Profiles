import unittest
from wiiu_library_profiles.core import CatalogError, parse_catalog, preview

def id_for(n):
    return f'00050000{n:08X}'

def source(n=400):
    return {'titles':[{'title_id':id_for(i),'device':'usb','app_type':'0x80000000','title_version':1} for i in range(n)]}

class ProfilesTests(unittest.TestCase):
    def test_over_300_are_preserved_in_catalog(self):
        t=parse_catalog(source(1000))
        self.assertEqual(len(t),1000)
        self.assertEqual(t[700].title_id,id_for(700))

    def test_game_not_mislabeled_as_native_wiiu(self):
        self.assertEqual(parse_catalog(source(1))[0].platform,'unclassified')

    def test_filter_only_manual_annotated_console(self):
        overrides={id_for(i):{'platform':'snes','origin':'injection'} for i in range(100)}
        t=parse_catalog(source(400),overrides=overrides)
        r=preview(t,{'name':'SNES','platforms':['snes']})
        self.assertEqual(r['visible_count'],100)
        self.assertEqual(r['hidden_count'],300)

    def test_include_id(self):
        t=parse_catalog(source(3))
        r=preview(t,{'name':'Favorites','include_ids':[id_for(2)]})
        self.assertEqual([v['title_id'] for v in r['titles']],[id_for(2)])

    def test_no_silent_truncation(self):
        t=parse_catalog(source(400),overrides={id_for(i):{'platform':'snes'} for i in range(400)})
        with self.assertRaisesRegex(CatalogError,'Split'):
            preview(t,{'name':'SNES','platforms':['snes']})

    def test_essential_system_title_is_unfilterable(self):
        base=source(1)
        base['titles'].append({'title_id':'0005001010040100','device':'mlc','app_type':'0x90000001'})
        r=preview(parse_catalog(base),{'name':'None','platforms':[]})
        self.assertEqual(r['visible_count'],0)
        self.assertEqual(r['pass_through_record_count'],1)
        self.assertEqual(r['launchable_record_count'],1)

    def test_duplicate_device_title_rejected(self):
        data=source(1);data['titles'].append(dict(data['titles'][0]))
        with self.assertRaisesRegex(CatalogError,'Duplicate'):
            parse_catalog(data)

    def test_same_id_two_devices_supported(self):
        data=source(1);data['titles'].append({**data['titles'][0],'device':'mlc'})
        self.assertEqual(len(parse_catalog(data)),2)

    def test_bad_title_id_rejected(self):
        with self.assertRaisesRegex(CatalogError,'title ID'):
            parse_catalog({'titles':[{'title_id':'DEADBEEF'}]})

    def test_refuse_system_reclassification(self):
        with self.assertRaisesRegex(CatalogError,'system'):
            parse_catalog({'titles':[{'title_id':'0005001010040100'}]},overrides={'0005001010040100':{'platform':'nes'}})

    def test_tag_union_and_origin_filter(self):
        data=source(3)
        t=parse_catalog(data,overrides={id_for(0):{'platform':'snes','origin':'official_vc','tags':['favorite']},id_for(1):{'platform':'gba','origin':'injection'}})
        r=preview(t,{'name':'All VC or Favorites','origins':['injection'],'tags':['favorite']})
        self.assertEqual({x['title_id'] for x in r['titles']},{id_for(0),id_for(1)})

    def test_platform_and_origin_intersection(self):
        data=source(3)
        t=parse_catalog(data,overrides={
            id_for(0):{'platform':'snes','origin':'official_vc'},
            id_for(1):{'platform':'snes','origin':'injection'},
            id_for(2):{'platform':'n64','origin':'injection'},
        })
        r=preview(t,{'name':'Injected SNES only','platforms':['snes'],'origins':['injection']})
        self.assertEqual([x['title_id'] for x in r['titles']],[id_for(1)])

    def test_exclusion_cannot_hide_system(self):
        data={'titles':[{'title_id':'0005001010040100'}]}
        r=preview(parse_catalog(data),{'name':'Empty','exclude_ids':['0005001010040100']})
        self.assertEqual(r['visible_count'],0)
        self.assertEqual(r['pass_through_record_count'],1)

    def test_system_data_and_overlay_protected_from_reclassification(self):
        for prefix in ('00050010','0005001B','00050030'):
            tid=prefix+'10040100'
            with self.subTest(prefix=prefix):
                with self.assertRaisesRegex(CatalogError,'system'):
                    parse_catalog({'titles':[{'title_id':tid}]},overrides={tid:{'platform':'snes'}})

    def test_nonlaunchable_ids_are_not_menu_icons_even_if_explicitly_included(self):
        ids=['0005000C10000100','0005000E10000100',
             '0005001010040100','0005001B10042300','000500301001010A']
        catalog={'titles':[{'title_id':tid} for tid in ids]}
        r=preview(parse_catalog(catalog),{'name':'Nonlaunchable','include_ids':ids})
        self.assertEqual(r['visible_count'],0)
        self.assertEqual(r['pass_through_record_count'],5)
        self.assertEqual(r['hidden_count'],0)

    def test_mixed_660_record_inventory_budget_counts_only_games(self):
        # Synthetic layout matching an observed 660-record console, not real IDs.
        records=source(511)['titles']
        groups=[('0005000C',26),('0005000E',68),('00050010',19),
                ('0005001B',20),('00050030',16)]
        for upper,count in groups:
            records.extend({'title_id':f'{upper}{i:08X}'} for i in range(count))
        t=parse_catalog({'titles':records})
        self.assertEqual(len(t),660)
        r=preview(t,{'name':'None'})
        self.assertEqual(r['launchable_record_count'],511)
        self.assertEqual(r['pass_through_record_count'],149)
        self.assertEqual(r['visible_count'],0)
        self.assertEqual(r['hidden_count'],511)

    def test_acp_metadata_names_are_used_without_guessing_vc_origin(self):
        t=parse_catalog({'titles':[{'title_id':id_for(25),'device':'usb',
            'app_type':'0x80000000','shortname_en':'Super Metroid',
            'shortname_es':'Metroid', 'meta_status':'ok',
            'product_code':'WUP-P-ABCD','content_platform':'WUP','region':2}]})[0]
        self.assertEqual(t.name,'Super Metroid')
        self.assertEqual(t.product_code,'WUP-P-ABCD')
        self.assertEqual(t.meta_status,'ok')
        self.assertEqual(t.platform,'wiiu')
        self.assertEqual(t.origin,'unknown')

    def test_acp_name_override_and_language_fallback(self):
        data={'titles':[{'title_id':id_for(3),'device':'usb',
                         'meta_status':'ok','shortname_ja':'日本語'},
                        {'title_id':id_for(4),'device':'usb',
                         'meta_status':'unavailable'}]}
        parsed=parse_catalog(data,overrides={id_for(3):{'name':'Override'}})
        self.assertEqual(parsed[0].name,'Override')
        self.assertEqual(parsed[1].name,'')
        self.assertEqual(parse_catalog(data)[0].name,'日本語')

    def test_acp_metadata_rejects_malformed_product_code(self):
        data={'titles':[{'title_id':id_for(1),'product_code':'A'*33}]}
        with self.assertRaisesRegex(CatalogError,'ACP product'):
            parse_catalog(data)

    def test_wiiu_product_code_family_hints(self):
        # Fake IDs, representative ACP code patterns; not a user's catalog.
        families={
            'WUP-N-FAZZ':'nes', 'WUP-N-JAZZ':'snes',
            'WUP-N-NAZZ':'n64', 'WUP-N-DAZZ':'nds',
            'WUP-N-PAZZ':'gba', 'WUP-N-PCZZ':'gba',
            'WUP-N-PNZZ':'tg16', 'WUP-N-PPZZ':'tg16',
            'WUP-N-MNZZ':'msx', 'WUP-N-VAZZ':'wii',
            'WUP-P-AAZZ':'wiiu', 'WUP-U-AAZZ':'wiiu',
            'WUP-N-ZZZZ':'unclassified'}
        for i,(code,expected) in enumerate(families.items()):
            with self.subTest(product_code=code):
                row={'title_id':id_for(i+1000),'device':'usb',
                     'app_type':'0x80000000','meta_status':'ok',
                     'product_code':code,'shortname_en':'Diagnostic'}
                parsed=parse_catalog({'titles':[row]})[0]
                self.assertEqual(parsed.platform,expected)
                self.assertEqual(parsed.origin,'unknown')

    def test_product_code_is_not_proof_of_original_rom_or_origin(self):
        row={'title_id':id_for(999),'app_type':'0x80000000',
             'product_code':'WUP-N-JAZZ','meta_status':'ok'}
        item=parse_catalog({'titles':[row]})[0]
        self.assertEqual(item.platform,'snes')
        self.assertEqual(item.origin,'unknown')
        # If SNES was just a donor, a manual override must take precedence.
        corrected=parse_catalog({'titles':[row]},overrides={
            id_for(999):{'platform':'gbc','origin':'injection'}})[0]
        self.assertEqual((corrected.platform,corrected.origin),('gbc','injection'))

    def test_no_acp_product_code_means_no_platform_guess(self):
        row={'title_id':id_for(1700),'meta_status':'unavailable',
             'app_type':'0x80000000','shortname_en':'Metroid'}
        parsed=parse_catalog({'titles':[row]})[0]
        self.assertEqual(parsed.platform,'unclassified')

if __name__=='__main__': unittest.main()
