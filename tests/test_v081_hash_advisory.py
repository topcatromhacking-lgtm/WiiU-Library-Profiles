import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "console_menu_profile_switch" / "src" / "main.cpp").read_text(encoding="utf-8")

class HashAdvisoryRegressionTest(unittest.TestCase):
    def test_barista_is_only_diagnostic(self):
        self.assertIn('barista_hash_match=check_save_digest();', SOURCE)
        self.assertNotIn('if(!check_save_digest())', SOURCE)
        self.assertNotIn('!barista_hash_match', SOURCE)
        self.assertNotIn('!save_ok', SOURCE)
        self.assertNotIn('BARISTA_HASH_MISMATCH', SOURCE)

    def test_security_gates_remain(self):
        self.assertIn('if(!exact_marker())return;', SOURCE)
        self.assertIn('if(!read_manifest())', SOURCE)
        self.assertIn('if(!current_account_matches())', SOURCE)
        self.assertIn('if(!wlp_ready(&stats))', SOURCE)
        self.assertIn('WUPS_FP_TARGET_PROCESS_WII_U_MENU', SOURCE)

    def test_versioned_csv_shows_advisory(self):
        self.assertIn('plugin_version,status,account,profile,barista_hash', SOURCE)
        self.assertIn('MISMATCH_OR_UNAVAILABLE', SOURCE)
        self.assertIn('0.8.1,', SOURCE)

if __name__ == "__main__":
    unittest.main()
