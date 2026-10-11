import tempfile
import unittest
from pathlib import Path
from wiiu_library_profiles.homebrew_scan import (
    HomebrewScanError, homebrew_title_id, scan_homebrew
)

class HomebrewScanTests(unittest.TestCase):
    def test_exact_hash_two_catalog_probes_confirmed_by_barista(self):
        self.assertEqual(homebrew_title_id('wiiu/apps/wiiu_library_profiles_probe/wiiu_library_profiles_probe.wuhb'),
                         '0005000FA12A4ECF')
        self.assertEqual(homebrew_title_id('wiiu/apps/wiiu_library_profiles_probe/wiiu_library_profiles_metadata_probe.wuhb'),
                         '0005000F4898F7DB')

    def test_title_id_changes_on_rename_case_or_move(self):
        a='wiiu/apps/tool/tool.wuhb'
        self.assertNotEqual(homebrew_title_id(a),
                            homebrew_title_id(a.replace('tool/tool.wuhb','Tool/tool.wuhb')))
        self.assertNotEqual(homebrew_title_id(a),homebrew_title_id('wiiu/apps/tool.wuhb'))

    def test_reject_unsafe_paths(self):
        for p in ['/wiiu/apps/x.wuhb','../wiiu/apps/x.wuhb','wiiu/apps/../x.wuhb',
                  'wiiu/apps\\x.wuhb', 'wiiu/apps//x.wuhb','sd:/wiiu/apps/x.wuhb']:
            with self.subTest(p=p),self.assertRaises(HomebrewScanError):
                homebrew_title_id(p)

    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.sd=Path(self.tmp.name)
        (self.sd/'wiiu/apps').mkdir(parents=True)
        self.addCleanup(self.tmp.cleanup)

    def file(self,rel):
        p=self.sd/rel
        p.parent.mkdir(parents=True,exist_ok=True)
        p.write_bytes(b'candidate')

    def test_one_level_scan_and_prefer_wuhb(self):
        self.file('wiiu/apps/example/example.wuhb')
        self.file('wiiu/apps/example/example.rpx')
        self.file('wiiu/apps/root.rpx')
        self.file('wiiu/apps/deeper/next/ignored.wuhb')
        result=scan_homebrew(self.sd)
        self.assertEqual(result['count'],2)
        self.assertEqual({x['relative_path'] for x in result['tool_candidates']},
             {'wiiu/apps/example/example.wuhb','wiiu/apps/root.rpx'})

    def test_hide_rpx_and_ignore_rules(self):
        self.file('wiiu/apps/x.rpx')
        self.file('wiiu/apps/y.wuhb')
        self.file('wiiu/apps/Xtreme/xt.wuhb')
        (self.sd/'wiiu/apps/.ignore').write_text('Y.WUHB\nxtreme/* # comment\n')
        r=scan_homebrew(self.sd,hide_all_rpx=True)
        self.assertEqual(r['count'],0)

    def test_temp_hidden_ignored(self):
        for name in ('temp.rpx','temp.wuhb','temp2.wuhb','.hidden.wuhb','_hidden.wuhb'):
            self.file('wiiu/apps/'+name)
        self.assertEqual(scan_homebrew(self.sd)['count'],0)

    def test_no_prefer_wuhb_includes_both(self):
        self.file('wiiu/apps/x.rpx')
        self.file('wiiu/apps/x.wuhb')
        self.assertEqual(scan_homebrew(self.sd,prefer_wuhb=False)['count'],2)

    def test_hide_all_homebrew_keeps_launcher_only(self):
        self.file('wiiu/apps/x.wuhb')
        self.file('wiiu/apps/homebrew_launcher/homebrew_launcher.wuhb')
        r=scan_homebrew(self.sd,hide_all_homebrew=True)
        self.assertEqual(r['count'],1)
        self.assertEqual(r['tool_candidates'][0]['relative_path'],
                         'wiiu/apps/homebrew_launcher/homebrew_launcher.wuhb')

    def test_missing_apps_is_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(HomebrewScanError):
                scan_homebrew(tmp)

if __name__=='__main__':
    unittest.main()
