import unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1]
BASE=(R/'console_menu_db_integrated/src/main.cpp').read_text()
NEW=(R/'console_menu_profile_preview/src/main.cpp').read_text()
class V12PreviewSafety(unittest.TestCase):
 def test_filter_hook_is_unchanged_except_optional_preview(self):
  key='DECL_FUNCTION(int32_t,MCP_TitleList'
  old=BASE[BASE.index(key):]
  new=NEW[NEW.index(key):]
  call=' /* Read-only preview: does not influence Barista or the proven filtering loop. */\n preview_capture(list,count,cap,&stats);\n'
  self.assertEqual(old,new.replace(call,''))
 def test_no_new_mcp_mount_or_write_barista(self):
  for prohibited in ('WHBMountSdCard(', 'MCP_Open(', 'MCP_TitleCount(', 'FSGetMountSource(', 'meta.xml'):
   self.assertNotIn(prohibited,NEW)
  self.assertEqual(NEW.count('real_MCP_TitleList('),1)
  self.assertIn('preview_opted_in=preview_marker_present();',NEW)
  self.assertIn('if(db_opted_in || preview_opted_in)db_load_once();',NEW)
  self.assertIn('if(preview_opted_in)',NEW)
 def test_separate_marker_and_safe_outputs(self):
  self.assertIn('ENABLE-V12-PREVIEW.txt',NEW)
  self.assertIn('ENABLE-WLP-V12-PREVIEW\\n',NEW)
  self.assertIn('active.wlps',NEW)
  self.assertIn('v12-preview-status.csv',NEW)
  self.assertIn('v12-preview-events.csv',NEW)
  self.assertIn('PREVIEW_PREFIX',NEW)
  self.assertIn('rename(part,path)',NEW)
  self.assertIn('FILE *existing=fopen(path,"rb")',NEW)
  self.assertIn('db_csv_text(f,meta.name,meta.name_len)',NEW)
 def test_preview_does_not_implicitly_enable_old_audits(self):
  self.assertIn('if(!db_opted_in || !stats',NEW)
  self.assertIn('if(inventory_io_failed || !wlp_inventory_candidate',NEW)
 def test_no_runtime_json_parser_or_rules_guess(self):
  self.assertIn('wlps_open(&view,raw,total)',NEW)
  self.assertIn('wlps_plan(&preview_spec,&db_view,ids,games,out,count,&stats)',NEW)
  self.assertIn('wlp_game_set_hash(ids,games,digest)',NEW)
  self.assertIn('if(!preview_opted_in || !list',NEW)
 def test_dependents_unchanged(self):
  for name in ('wlp_filter_core.h','wlp_sha256.h','wlp_inventory_core.h','wlpdb_core.h','wlp_game_set.h'):
   self.assertEqual((R/'console_menu_profile_preview/src'/name).read_bytes(),
                    (R/'console_menu_db_integrated/src'/name).read_bytes())
if __name__=='__main__':unittest.main()
