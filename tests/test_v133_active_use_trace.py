import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from v133_active_use_trace import load_trace, verify, TraceInvalid, SAVE_BYTES

ACCOUNT = "80000001"
BOOT = "testing-boot-token"


def event(event, **kwargs):
    return {"seq": -1, "boot": BOOT, "event": event, **kwargs}


class TraceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.backup = bytes((i * 73 + 19) % 256 for i in range(SAVE_BYTES))

    def trace(self, *, chunk=10000):
        events = [
            event("SAVE_INIT", menu_title=True, sd_mounted=True, homebrew_visible=True, account=ACCOUNT),
            event("LAYER_REGISTER", layer="homp_save_redirection", result="SUCCESS", handle=11),
            event("MENU_OPEN", layer="homp_save_redirection", handle=11, file="BaristaAccountSaveFile.dat", physical_source="SD_REDIRECT_ACCOUNT_SAVE", file_handle=55, result="SUCCESS")
        ]
        for offset in range(0, SAVE_BYTES, chunk):
            data = self.backup[offset:offset+chunk]
            events.append(event("MENU_READ", file_handle=55, offset=offset, length=len(data), sha256=hashlib.sha256(data).hexdigest(), result="SUCCESS"))
        events += [event("MENU_CLOSE", file_handle=55, result="SUCCESS"), event("LAYER_REMOVE", handle=11), event("SESSION_END")]
        for n, e in enumerate(events):
            e["seq"] = n
        return events

    def result(self, events):
        return verify(events, self.backup, ACCOUNT)

    def assert_state(self, events, status, detail=None):
        r = self.result(events)
        self.assertEqual(r["status"], status, r)
        if detail:
            self.assertEqual(r["detail"], detail)

    def test_complete_event_trace_passes(self):
        self.assert_state(self.trace(), "ACTIVE_USE_VERIFIED_BY_TRACE")

    def test_read_per_byte_passes(self):
        self.assert_state(self.trace(chunk=4096), "ACTIVE_USE_VERIFIED_BY_TRACE")

    def test_single_full_read_passes(self):
        self.assert_state(self.trace(chunk=SAVE_BYTES), "ACTIVE_USE_VERIFIED_BY_TRACE")

    def test_missing_end_unproven(self):
        self.assert_state(self.trace()[:-1], "ACTIVE_USE_UNPROVEN")

    def test_missing_open_unproven(self):
        ev = [x for x in self.trace() if x["event"] != "MENU_OPEN"]
        self.assert_state(ev, "ACTIVE_USE_UNPROVEN")

    def test_missing_close_unproven(self):
        ev = [x for x in self.trace() if x["event"] != "MENU_CLOSE"]
        self.assert_state(ev, "ACTIVE_USE_UNPROVEN")

    def test_missing_layer_registration_unproven(self):
        ev = [x for x in self.trace() if x["event"] != "LAYER_REGISTER"]
        self.assert_state(ev, "ACTIVE_USE_UNPROVEN")

    def test_wrong_source_unproven(self):
        t=self.trace();t[2]["physical_source"]="SD_CANDIDATE_INDEPENDENT_READ"
        self.assert_state(t, "ACTIVE_USE_UNPROVEN")

    def test_wrong_layer_name_unproven(self):
        t=self.trace();t[1]["layer"]="hacked"
        self.assert_state(t, "LAYER_REGISTRATION_FAILED")

    def test_mismatched_read_bytes(self):
        t=self.trace();t[3]["sha256"]="0"*64
        self.assert_state(t, "SOURCE_MISMATCH")

    def test_partial_read_coverage(self):
        t=self.trace();t.pop(4)
        self.assert_state(t, "ACTIVE_USE_UNPROVEN", "PARTIAL_READ_COVERAGE")

    def test_layer_removed_before_reads(self):
        t=self.trace(); r=t.pop(-2);t.insert(3,r)
        self.assert_state(t, "ACTIVE_USE_UNPROVEN")

    def test_account_switch_invalid(self):
        t=self.trace();t.insert(-2,event("ACCOUNT_CHANGE",account="80000009"))
        self.assert_state(t, "SESSION_INVALID")

    def test_account_at_init_mismatch(self):
        t=self.trace();t[0]["account"]="80000009"
        self.assert_state(t, "SESSION_INVALID")

    def test_console_not_menu(self):
        t=self.trace();t[0]["menu_title"]=False
        self.assert_state(t, "LAYER_NOT_ACTIVE")

    def test_sd_missing(self):
        t=self.trace();t[0]["sd_mounted"]=False
        self.assert_state(t, "LAYER_NOT_ACTIVE")

    def test_hidden_homebrew(self):
        t=self.trace();t[0]["homebrew_visible"]=False
        self.assert_state(t, "LAYER_NOT_ACTIVE")

    def test_register_failed(self):
        t=self.trace();t[1]["result"]="ERROR"
        self.assert_state(t, "LAYER_REGISTRATION_FAILED")

    def test_bogus_handle(self):
        t=self.trace();t[2]["file_handle"]=0
        self.assert_state(t, "ACTIVE_USE_UNPROVEN")

    def test_incorrect_length(self):
        t=self.trace();t[3]["length"]=SAVE_BYTES*3
        self.assert_state(t, "ACTIVE_USE_UNPROVEN")

    def test_invalid_hex_hash(self):
        t=self.trace();t[3]["sha256"]="ZZ"
        self.assert_state(t, "ACTIVE_USE_UNPROVEN")

    def test_reader_error(self):
        t=self.trace();t[3]["result"]="FAILED"
        self.assert_state(t, "ACTIVE_USE_UNPROVEN")

    def test_out_of_order_session_events(self):
        t=self.trace();t[0],t[1]=t[1],t[0]
        self.assert_state(t,"SESSION_INVALID")

    def test_second_layer_register_invalid(self):
        t=self.trace();t.insert(4,event("LAYER_REGISTER",layer="homp_save_redirection",handle=2,result="SUCCESS"))
        self.assert_state(t,"SESSION_INVALID")

    def test_overlap_reads_still_matches(self):
        t=self.trace();start=t[3].copy(); t.insert(5,start)
        self.assert_state(t,"ACTIVE_USE_VERIFIED_BY_TRACE")

    def test_late_read_after_close_unproven(self):
        t=self.trace();read=t[3].copy();t.insert(-2,read)
        self.assert_state(t,"ACTIVE_USE_UNPROVEN")

    def test_invalid_backup_size(self):
        self.assertEqual(verify(self.trace(),b"empty",ACCOUNT)["status"],"INVALID_BACKUP")

    def test_unknown_account_format(self):
        self.assertEqual(verify(self.trace(),self.backup,"account")["status"],"INVALID_ARGUMENT")

    def test_duplicate_sequence_rejected(self):
        t=self.trace();t[1]["seq"]=t[0]["seq"]
        with tempfile.TemporaryDirectory() as temp:
            f=Path(temp)/"trace.jsonl"
            f.write_text("\n".join(json.dumps(e) for e in t)+"\n",encoding="utf-8")
            with self.assertRaises(TraceInvalid):load_trace(f)

    def test_schema_rejects_extra_sensitive_fields(self):
        t=self.trace();t[2]["serial"]="SHOULD-NOT-BE-HERE"
        with tempfile.TemporaryDirectory() as temp:
            f=Path(temp)/"trace.jsonl"
            f.write_text("\n".join(json.dumps(e) for e in t)+"\n",encoding="utf-8")
            with self.assertRaises(TraceInvalid):load_trace(f)

    def test_mixed_boot_rejected(self):
        t=self.trace();t[2]["boot"]="different-boot-token"
        with tempfile.TemporaryDirectory() as temp:
            f=Path(temp)/"trace.jsonl"
            f.write_text("\n".join(json.dumps(e) for e in t)+"\n",encoding="utf-8")
            with self.assertRaises(TraceInvalid):load_trace(f)


if __name__ == "__main__": unittest.main()
