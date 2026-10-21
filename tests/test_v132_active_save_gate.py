import csv
import hashlib
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'src'))
import v13_pair_generator as gen
import v132_active_save_gate as gate
from test_v13_pair_generator import mk_template,mk_fixture


def attestation(b:bytes):
 return {'stage':'V132','status':'STRUCTURE_OK',
  'source_role':gate.ROLE,
  'source_path':'fs:/vol/external01/wiiu/homebrew_on_menu_plugin/SERIAL123/save/80000001/BaristaAccountSaveFile.dat',
  'account':'80000001','bytes':str(len(b)),
  'sha256':gen.sha(b).upper(),
  'fixed_prefix_sha256':gen.sha(b[:5040]),
  'preserved_suffix_sha256':gen.sha(b[9600:]),
  'game_slots_used':'285','folders_empty':'60','fixed_icons':'15',
  'redirected_copy_is_active':'UNPROVEN'}

class GateTests(unittest.TestCase):
 def setUp(self):
  self.orig=mk_template();self.a=attestation(self.orig)
 def check(self,b=None,a=None,second=None):
  if b is None:b=self.orig
  if a is None:a=self.a
  if second is None:second=b
  return gate.check_two_backups(b,second,a)
 def test_good_double_backup_is_still_not_deployable(self):
  report=self.check()
  self.assertEqual(report['account'],'80000001')
  self.assertFalse(report['install_authorized'])
  self.assertFalse(report['source_attested_as_active_by_redirection_layer'])
  self.assertEqual(report['source_size'],130116)
 def test_wrong_account_rejected(self):
  a=dict(self.a,account='80000009')
  with self.assertRaisesRegex(gen.Reject,'account mismatch'):self.check(a=a)
 def test_unknown_provenance_rejected(self):
  a=dict(self.a,source_role='GENERIC_SD_FILE')
  with self.assertRaisesRegex(gen.Reject,'source role'):self.check(a=a)
 def test_wrong_path_account_rejected(self):
  a=dict(self.a,source_path=self.a['source_path'].replace('80000001','80000009'))
  with self.assertRaisesRegex(gen.Reject,'source path'):self.check(a=a)
 def test_unproven_redirect_must_be_explicit(self):
  a=dict(self.a,redirected_copy_is_active='TRUE')
  with self.assertRaisesRegex(gen.Reject,'unsupported claim'):self.check(a=a)
 def test_stale_attestation_is_rejected(self):
  a=dict(self.a,sha256='0'*64)
  with self.assertRaisesRegex(gen.Reject,'digest differs'):self.check(a=a)
 def test_prefix_mismatch_rejected(self):
  a=dict(self.a,fixed_prefix_sha256='0'*64)
  with self.assertRaisesRegex(gen.Reject,'prefix digest'):self.check(a=a)
 def test_suffix_mismatch_rejected(self):
  a=dict(self.a,preserved_suffix_sha256='0'*64)
  with self.assertRaisesRegex(gen.Reject,'suffix digest'):self.check(a=a)
 def test_two_backups_differ_rejected(self):
  b=bytearray(self.orig);b[9607]^=1
  with self.assertRaisesRegex(gen.Reject,'differ byte-for-byte'):self.check(second=bytes(b))
 def test_extra_homebrew_rejected(self):
  b=mk_template(extra=True)
  with self.assertRaisesRegex(gen.Reject,'outside first 15'):self.check(b=b,a=attestation(b))
 def test_folders_not_empty_rejected(self):
  b=bytearray(self.orig);struct.pack_into('>Q',b,gen.FOLDER_START,0x0005000010101111);b=bytes(b)
  with self.assertRaisesRegex(gen.Reject,'Populated folder'):self.check(b=b,a=attestation(b))
 def test_path_guard(self):
  self.assertFalse(gate._safe_path(Path('/mnt/sd/wiiu/')))
  self.assertTrue(gate._safe_path(Path('/mnt/data/preflight-checks')))
 def test_double_backup_pair_277_vs_511_protocol(self):
  with tempfile.TemporaryDirectory() as d:
   ids,prev,inv,audit=mk_fixture(d,277)
   with inv.open('a',newline='') as f:
    w=csv.writer(f)
    for j in range(7):w.writerow([f'{0x0005000F01000001+j:016X}','HOMEBREW_SYNTHETIC'])
   fingerprint=gen.sha(b''.join(int(x,16).to_bytes(8,'big') for x in ids))
   r,candidate,wlpa=gate.make_checked_pair(self.orig,self.orig,self.a,preview=prev,inventory=inv,audit=audit,expected_game_set_sha256=fingerprint)
   self.assertEqual(r['selected_games'],277)
   self.assertEqual(r['wlpa_count'],284)
   self.assertEqual(r['independent_verifier_result'],'INDEPENDENT_OFFLINE_PAIR_VALIDATED_NOT_INSTALL_AUTHORIZATION')
   self.assertFalse(r['install_authorized'])
   self.assertEqual(len(candidate),130116)
   self.assertEqual(len(wlpa),44+284*8+4)
 def test_changed_game_set_rejected(self):
  with tempfile.TemporaryDirectory() as d:
   ids,prev,inv,audit=mk_fixture(d,277)
   with self.assertRaisesRegex(gen.Reject,'Installed game set'):
    gate.make_checked_pair(self.orig,self.orig,self.a,preview=prev,inventory=inv,audit=audit,expected_game_set_sha256='A'*64)
if __name__=='__main__':unittest.main()
