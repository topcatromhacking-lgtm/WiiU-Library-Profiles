from pathlib import Path
import unittest
R=Path(__file__).resolve().parents[1]
OLD=(R/'console_menu_profile_preview/src/main.cpp').read_text()
NEW=(R/'console_menu_save_attestation/src/main.cpp').read_text()
class AttestationIntegration(unittest.TestCase):
 def test_existing_menu_hook_unchanged(self):
  key='DECL_FUNCTION(int32_t,MCP_TitleList'
  orig=OLD[OLD.index(key):];revised=NEW[NEW.index(key):]
  addition=' /* Optional read-only attestation of redirected Barista candidate. */\n attest_capture(&stats);\n'
  self.assertEqual(orig,revised.replace(addition,''))
 def test_existing_core_same(self):
  for name in ('wlp_filter_core.h','wlp_sha256.h','wlp_inventory_core.h','wlpdb_core.h','wlp_game_set.h','wlps_core.h'):
   self.assertEqual((R/'console_menu_profile_preview/src'/name).read_bytes(),
                    (R/'console_menu_save_attestation/src'/name).read_bytes())
 def test_exact_account_gate(self):
  self.assertIn('target_account!=UINT32_C(0x80000001)',NEW)
  self.assertIn('if(!attest_opted_in||attest_finished',NEW)
  self.assertIn('attest_opted_in=attest_marker_present();',NEW)
 def test_read_only_save_with_no_extra_services(self):
  self.assertIn('FILE *f=fopen(path,"rb")',NEW)
  self.assertIn('wlp_barista_inspect(buffer,n,&info)',NEW)
  self.assertNotIn('fopen(path,"wb")',NEW)
  for token in ('MCP_Open(', 'MCP_TitleCount(', 'WHBMountSdCard(', 'FSGetMountSource(', 'Mocha_MountFS('):
   self.assertNotIn(token,NEW)
  self.assertEqual(NEW.count('real_MCP_TitleList('),1)
 def test_explicit_attestation_limits(self):
  for item in ('ENABLE-V132-ATTEST.txt','ENABLE-WLP-V132-ATTEST\\n',
               'v132-barista-attest.csv','v132-fixed-icons.csv',
               'HOMEBREW_ON_MENU_SD_REDIRECTED_CANDIDATE',
               'UNPROVEN','STRUCTURE_OK','BARISTA_STRUCTURE_UNSUPPORTED'):
   self.assertIn(item,NEW)
if __name__=='__main__':unittest.main()
