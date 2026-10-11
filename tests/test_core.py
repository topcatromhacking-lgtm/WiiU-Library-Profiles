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
        self.assertEqual([t['title_id'] for t in r['titles']],['0005001010040100'])

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
        self.assertEqual(r['visible_count'],1)

if __name__=='__main__': unittest.main()
