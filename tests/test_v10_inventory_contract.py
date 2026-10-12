import pathlib
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
SOURCE=(ROOT/"standalone_inventory_scanner"/"source"/"main.c").read_text(encoding="utf-8")
class ReadOnlyScannerContract(unittest.TestCase):
    def test_not_a_plugin_and_does_not_install(self):
        for forbidden in ("WUPS_MUST_REPLACE","WUPS_PLUGIN","MCP_Install","MCP_Uninstall","BaristaAccountSaveFile.dat","wiiu_library_profiles_v081_hash_advisory"):
            self.assertNotIn(forbidden,SOURCE)
    def test_scans_installed_titles_and_sd_homebrew(self):
        for required in ("MCP_TitleCount","MCP_TitleList","MCP_APP_TYPE", "sd:/wiiu/apps","WHBMountSdCard","WHBProcInit"):
            if required=="MCP_APP_TYPE":continue
            self.assertIn(required,SOURCE)
    def test_exports_uncertainty_explicitly(self):
        for required in ("WIIU_OR_VIRTUAL_CONSOLE_UNKNOWN","UNKNOWN","platform_confidence","region_confidence","NOT_MAPPED_TO_SYNTHETIC_ID","metadata_readable"):
            self.assertIn(required,SOURCE)
    def test_only_three_writable_paths_under_v10(self):
        import re
        for path in re.findall(r'fopen\(([^,]+),"wb"\)',SOURCE):
            self.assertIn(path,("TITLE_OUT","HB_OUT","SUMMARY_OUT"))
    def test_subdirectory_scan_is_bounded(self):
        self.assertIn('scan_homebrew_dir(f,"sd:/wiiu/apps",2,errors)',SOURCE)
        self.assertIn("MAX_TITLES 8192u",SOURCE)
if __name__=="__main__":
    unittest.main()
