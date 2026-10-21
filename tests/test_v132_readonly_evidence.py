"""Synthetic v1.3.2 read-only report reconciliation and anti-tampering tests."""
import copy
import hashlib
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from v13_pair_generator import Reject, sha
from v132_readonly_evidence import verify_snapshot
from test_v13_pair_generator import mk_template

class EvidenceTests(unittest.TestCase):
    def setUp(self):
        self.save = mk_template()
        self.att = {
            "stage":"V132", "status":"STRUCTURE_OK", "source_role":"HOMEBREW_ON_MENU_SD_REDIRECTED_CANDIDATE",
            "account":"80000001", "source_path":"fs:/vol/external01/wiiu/homebrew_on_menu_plugin/TESTSERIAL/save/80000001/BaristaAccountSaveFile.dat",
            "bytes":str(len(self.save)), "sha256":sha(self.save).upper(),
            "fixed_prefix_sha256":sha(self.save[:5040]), "preserved_suffix_sha256":sha(self.save[9600:]),
            "game_slots_used":"285", "folders_empty":"60", "fixed_icons":"15", "redirected_copy_is_active":"UNPROVEN"
        }
        self.icons = []
        for i in range(15):
            a, t = struct.unpack_from(">QI", self.save, i * 16)
            self.icons.append({"slot":str(i + 1), "title_id":f"{a:016X}", "record_type":str(t)})
        self.menu = {"plugin_version":"0.8.1", "status":"FILTER_APPLIED", "account":"80000001", "profile":"wiiu",
                     "barista_hash":"MISMATCH_OR_UNAVAILABLE", "records":"603", "capacity":"8000",
                     "expected_games":"285", "found_games":"285", "expected_homebrew":"7", "found_homebrew":"7"}

    def check(self):
        return verify_snapshot(self.save, self.att, self.icons, self.menu)

    def test_valid_snapshot_is_not_active_use_proof(self):
        result=self.check()
        self.assertEqual(result["result"], "READ_ONLY_EVIDENCE_INTEGRITY_PASS")
        self.assertEqual(result["powered_off_backups_independently_verified"],1)
        self.assertFalse(result["install_authorized"])
        self.assertFalse(result["active_redirection_source_verified"])

    def test_reject_stale_backup(self):
        self.att["sha256"] = "0" * 64
        with self.assertRaisesRegex(Reject,"SHA-256 differs"):self.check()

    def test_reject_icon_value_tamper(self):
        self.icons[1]["title_id"] = "0005001010000000"
        with self.assertRaisesRegex(Reject,"Title ID/type mismatch"):self.check()

    def test_reject_icon_type_tamper(self):
        self.icons[1]["record_type"] = "2"
        with self.assertRaisesRegex(Reject,"Title ID/type mismatch"):self.check()

    def test_reject_out_of_order_icons(self):
        self.icons[2]["slot"] = "9"
        with self.assertRaisesRegex(Reject,"out-of-order"):self.check()

    def test_reject_missing_icon(self):
        self.icons.pop()
        with self.assertRaisesRegex(Reject,"icon count"):self.check()

    def test_reject_unsupported_active_claim(self):
        self.att["redirected_copy_is_active"] = "PROVEN"
        with self.assertRaisesRegex(Reject,"unsupported active-source"):self.check()

    def test_reject_menu_wrong_account(self):
        self.menu["account"] = "80000002"
        with self.assertRaisesRegex(Reject,"v0.8.1 account"):self.check()

    def test_reject_menu_missing_game(self):
        self.menu["found_games"] = "284"
        with self.assertRaisesRegex(Reject,"fewer games"):self.check()

    def test_reject_menu_game_count_different_to_save(self):
        self.menu["found_games"] = self.menu["expected_games"] = "284"
        with self.assertRaisesRegex(Reject,"differs from Barista"):self.check()

    def test_reject_menu_homebrew_mismatch(self):
        self.menu["found_homebrew"] = "6"
        with self.assertRaisesRegex(Reject,"Homebrew count"):self.check()

    def test_reject_unknown_report_hash_field(self):
        self.menu["barista_hash"] = "unrecognized"
        with self.assertRaisesRegex(Reject,"unknown menu report hash"):self.check()

    def test_reject_wrong_report_hash(self):
        self.menu["barista_hash"] = "0"*64
        with self.assertRaisesRegex(Reject,"Barista SHA-256 mismatch"):self.check()

    def test_matching_report_hash_still_not_proof_of_active_use(self):
        self.menu["barista_hash"] = sha(self.save)
        r=self.check()
        self.assertFalse(r["active_redirection_source_verified"])

    def test_reject_wrong_prefix(self):
        self.att["fixed_prefix_sha256"] = "0"*64
        with self.assertRaisesRegex(Reject,"protected icon region"):self.check()

    def test_reject_wrong_suffix(self):
        self.att["preserved_suffix_sha256"] = "0"*64
        with self.assertRaisesRegex(Reject,"preserved Barista suffix"):self.check()

    def test_reject_different_save_path(self):
        self.att["source_path"] = "fs:/vol/external01/wiiu/homebrew_on_menu_plugin/TESTSERIAL/save/80000009/BaristaAccountSaveFile.dat"
        with self.assertRaisesRegex(Reject,"source path"):self.check()

    def test_reject_short_snapshot(self):
        self.save=self.save[:-1]
        with self.assertRaisesRegex(Reject,"size differs"):self.check()

if __name__ == "__main__":unittest.main()
