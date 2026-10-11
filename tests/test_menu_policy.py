import unittest
from wiiu_library_profiles.core import CatalogError, parse_catalog
from wiiu_library_profiles.menu_policy import plan_fixed_menu, MENU_SLOTS


def tid(n):
    return f'00050000{n:08X}'


def inventory(blocks):
    titles=[]
    next_id=100
    for platform,count in blocks:
        for i in range(count):
            title=tid(next_id)
            titles.append({'title_id':title,'device':'usb','app_type':'0x80000000',
                           'shortname_en':f'{platform.upper()} {i:04d}'})
            next_id+=1
    overrides={}
    k=0
    next_id=100
    for platform,count in blocks:
        for i in range(count):
            overrides[tid(next_id)]={'platform':platform}
            next_id+=1
    return parse_catalog({'titles':titles},overrides=overrides)


def fixed(eight=8):
    return [{'position':i,'title_id':f'00050010{i+500:08X}'} for i in range(eight)]


def tools(n=9):
    return [{'key':f'sd:/wiiu/apps/app{i}.wuhb',
             'title_id':f'0005000F{i+100:08X}', 'name':f'Tool {i}'}
            for i in range(n)]


def plan(titles,priority,selected=None,known=None,base_fixed=None,order=None):
    return plan_fixed_menu(
        titles,priority=priority,
        fixed_system_slots=fixed() if base_fixed is None else base_fixed,
        tool_candidates=tools() if known is None else known,
        selected_tool_keys=[x['key'] for x in tools(7)] if selected is None else selected,
        saved_order=order
    )


class FixedPagePriorityTests(unittest.TestCase):
    def test_capacity_is_15_plus_19_pages_of_15(self):
        r=plan(inventory([('wiiu',270)]),['wiiu'])
        self.assertEqual(MENU_SLOTS,300)
        self.assertEqual(len(r['first_page']),15)
        self.assertEqual(len(r['software_pages']),285)
        self.assertEqual(r['visible_tool_count'],7)
        self.assertEqual(r['fixed_system_count'],8)

    def test_280_wiiu_10_wii_shows_only_five_wii(self):
        r=plan(inventory([('wiiu',280),('wii',10)]),['wiiu','wii'])
        self.assertEqual(r['visible_software_count'],285)
        self.assertEqual([d['displayed'] for d in r['per_platform']],[280,5])
        self.assertEqual(r['selected_platform_overflow'],5)
        self.assertEqual(len(r['hidden_installed_game_ids']),5)

    def test_270_wiiu_10_wii_10_nds_shows_five_nds(self):
        r=plan(inventory([('wiiu',270),('wii',10),('nds',10)]),
               ['wiiu','wii','nds'])
        self.assertEqual([d['displayed'] for d in r['per_platform']],[270,10,5])
        self.assertEqual(r['selected_platform_overflow'],5)

    def test_only_fifteen_wiiu_no_unselected_refilling(self):
        t=inventory([('wiiu',15),('nes',50)])
        r=plan(t,['wiiu'])
        self.assertEqual(r['visible_software_count'],15)
        self.assertEqual(r['empty_software_count'],270)
        self.assertEqual(r['not_selected_game_count'],50)
        self.assertEqual(r['software_pages'][0]['position'],15)
        self.assertEqual(r['software_pages'][14]['position'],29)
        self.assertEqual(r['software_pages'][15]['kind'],'empty_software')
        self.assertEqual(len(r['hidden_installed_game_ids']),50)

    def test_only_fifteen_nes_still_only_page_two(self):
        t=inventory([('wiiu',30),('nes',15)])
        r=plan(t,['nes'])
        self.assertEqual(r['visible_software_count'],15)
        self.assertEqual(r['empty_software_count'],270)
        self.assertEqual({s['platform'] for s in r['software_pages']
                          if s['kind']=='software'},{'nes'})

    def test_fewer_tools_propose_not_implement_folder_placeholders(self):
        known=tools(4)
        selected=[x['key'] for x in known[:3]]
        r=plan(inventory([('wiiu',15)]),['wiiu'],selected,known)
        self.assertEqual(r['fixed_system_count'],8)
        self.assertEqual(r['visible_tool_count'],3)
        self.assertEqual(r['placeholder_folder_count'],4)
        self.assertEqual(sum(s['kind']=='placeholder_folder' for s in r['first_page']),4)

    def test_more_than_available_tools_requires_selection(self):
        known=tools(9)
        with self.assertRaisesRegex(CatalogError,'Only 7'):
            plan(inventory([]),[],[x['key'] for x in known[:8]],known)

    def test_missing_homebrew_or_tool_rejected(self):
        with self.assertRaisesRegex(CatalogError,'missing'):
            plan(inventory([]),[],['sd:/wiiu/apps/no-longer-installed.wuhb'])

    def test_unknown_games_do_not_appear_unless_selected(self):
        t=inventory([('wiiu',2),('unclassified',1)])
        r=plan(t,['wiiu'])
        self.assertEqual(r['visible_software_count'],2)
        self.assertEqual(len(r['unclassified_game_ids']),1)
        r2=plan(t,['wiiu','unclassified'])
        self.assertEqual(r2['visible_software_count'],3)

    def test_eur_and_jpn_title_ids_are_not_discarded(self):
        # IDs are completely independent of the region; classification drives the result.
        t=inventory([('nes',1),('snes',1)])
        r=plan(t,['snes','nes'])
        self.assertEqual(r['visible_software_count'],2)
        self.assertEqual([x['platform'] for x in r['software_pages'][:2]],['snes','nes'])

    def test_keep_user_order_and_append_new_install(self):
        t=inventory([('snes',3)])
        old_ids=[x.title_id for x in t]
        order={'snes':[old_ids[2],old_ids[0]]}
        r=plan(t,['snes'],order=order)
        self.assertEqual(r['software_title_ids_in_order'],[old_ids[2],old_ids[0],old_ids[1]])
        t2=inventory([('snes',4)])
        updated=plan(t2,['snes'],order=order)
        self.assertEqual(updated['software_title_ids_in_order'][:3],
                         [old_ids[2],old_ids[0],old_ids[1]])
        self.assertEqual(updated['software_title_ids_in_order'][3],tid(103))

    def test_system_update_dlc_are_not_slots(self):
        entries=inventory([('wiiu',1)])
        extended=parse_catalog({'titles':[
            {'title_id':'0005000E00001234'},
            {'title_id':'0005000C00001234'},
            {'title_id':'0005001010040100'}
        ]})
        r=plan(entries+extended,['wiiu'])
        self.assertEqual(r['visible_software_count'],1)
        self.assertEqual(r['nonlaunchable_records_pass_through'],3)

    def test_fixed_system_slots_cannot_overflow_page(self):
        with self.assertRaisesRegex(CatalogError,'outside first page'):
            plan(inventory([]),[],base_fixed=[{
                'position':15,'title_id':'0005001010040100'}])

    def test_fixed_system_slots_cannot_overlap(self):
        with self.assertRaisesRegex(CatalogError,'Duplicate fixed'):
            plan(inventory([]),[],base_fixed=[
                {'position':0,'title_id':'0005001010040100'},
                {'position':0,'title_id':'0005001010040200'}])

    def test_double_installed_device_title_appears_once(self):
        t=inventory([('wiiu',1)])
        from dataclasses import replace
        t.append(replace(t[0],device='mlc'))
        r=plan(t,['wiiu'])
        self.assertEqual(r['visible_software_count'],1)


if __name__=='__main__':
    unittest.main()
