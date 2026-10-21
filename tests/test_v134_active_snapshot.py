"""Active layer snapshot semantics for actual Menu lifetime, synthetic only."""
import hashlib
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from v133_active_use_trace import verify, SAVE_BYTES

ACC="80000001"
BACKUP=bytes((i*17+7)%256 for i in range(SAVE_BYTES))

def trace():
    ev=[
      ("SAVE_INIT",dict(menu_title=True,sd_mounted=True,homebrew_visible=True,account=ACC)),
      ("LAYER_REGISTER",dict(layer="homp_save_redirection",result="SUCCESS",handle=11)),
      ("MENU_OPEN",dict(layer="homp_save_redirection",handle=11,file="BaristaAccountSaveFile.dat",physical_source="SD_REDIRECT_ACCOUNT_SAVE",file_handle=55,result="SUCCESS")),
      ("MENU_READ",dict(file_handle=55,offset=0,length=SAVE_BYTES,sha256=hashlib.sha256(BACKUP).hexdigest(),result="SUCCESS")),
      ("MENU_CLOSE",dict(file_handle=55,result="SUCCESS")),
      ("ACTIVE_SNAPSHOT",dict(handle=11,still_active=True)),
      ("SESSION_END",dict())]
    return [dict(seq=i,boot="snapshot-session-001",event=kind,**props) for i,(kind,props) in enumerate(ev)]

class SnapshotTests(unittest.TestCase):
 def status(self,rows):return verify(rows,BACKUP,ACC)
 def test_active_snapshot_proves_full_read(self):self.assertEqual(self.status(trace())["status"],"ACTIVE_USE_VERIFIED_BY_TRACE")
 def test_snapshot_missing(self):
    t=[x for x in trace() if x["event"]!="ACTIVE_SNAPSHOT"]
    self.assertEqual(self.status(t)["status"],"ACTIVE_USE_UNPROVEN")
 def test_snapshot_before_close_invalid(self):
    t=trace();t[-3],t[-2]=t[-2],t[-3]
    self.assertEqual(self.status(t)["status"],"SESSION_INVALID")
 def test_snapshot_false_invalid(self):
    t=trace();t[-2]["still_active"]=False
    self.assertEqual(self.status(t)["status"],"SESSION_INVALID")
 def test_snapshot_wrong_handle_invalid(self):
    t=trace();t[-2]["handle"]=12
    self.assertEqual(self.status(t)["status"],"SESSION_INVALID")
 def test_snapshot_repeated_invalid(self):
    t=trace();t.insert(-1,t[-2].copy())
    self.assertEqual(self.status(t)["status"],"SESSION_INVALID")
 def test_after_snapshot_remove_invalid(self):
    t=trace();t.insert(-1,dict(seq=99,boot="snapshot-session-001",event="LAYER_REMOVE",handle=11))
    self.assertEqual(self.status(t)["status"],"SESSION_INVALID")
 def test_snapshot_not_complete_invalid(self):
    t=trace();t[3]["length"]=100
    t[3]["sha256"]=hashlib.sha256(BACKUP[:100]).hexdigest()
    self.assertEqual(self.status(t)["status"],"ACTIVE_USE_UNPROVEN")
 def test_layer_removed_then_snapshot_invalid(self):
    t=trace();t.insert(-2,dict(seq=99,boot="snapshot-session-001",event="LAYER_REMOVE",handle=11))
    self.assertEqual(self.status(t)["status"],"SESSION_INVALID")
 def test_account_changes_before_snapshot_invalid(self):
    t=trace();t.insert(-2,dict(seq=99,boot="snapshot-session-001",event="ACCOUNT_CHANGE",account="80000002"))
    self.assertEqual(self.status(t)["status"],"SESSION_INVALID")
 def test_bad_backup_detected(self):
    b=bytearray(BACKUP);b[22]^=0x44
    self.assertEqual(verify(trace(),b,ACC)["status"],"SOURCE_MISMATCH")
 def test_truncated_trace_unproven(self):
    self.assertEqual(self.status(trace()[:-1])["status"],"ACTIVE_USE_UNPROVEN")

if __name__=="__main__":unittest.main()
