import unittest
from pathlib import Path
root=Path(__file__).resolve().parents[1]
v10=(root/'console_menu_inventory_embedded/src/main.cpp').read_text()
v111=(root/'console_menu_db_integrated/src/main.cpp').read_text()
class SnapshotSafety(unittest.TestCase):
 def test_unchanged_core_menu_filter(self):
  key='DECL_FUNCTION(int32_t,MCP_TitleList'
  old=v10[v10.index(key):]
  new=v111[v111.index(key):]
  addition=' /* Read-only database join; no changes to allowlist compaction below. */\n db_audit_capture(list,count,cap,&stats);\n'
  self.assertEqual(old,new.replace(addition,''))
 def test_no_extra_mount_or_mcp_or_barista_writes(self):
  for banned in ('WHBMountSdCard(', 'MCP_Open(', 'MCP_TitleCount(', 'FSGetMountSource(', 'meta.xml'):
   self.assertNotIn(banned,v111)
  self.assertEqual(v111.count('real_MCP_TitleList('),1)
 def test_trigger_only_on_game_title_set_and_database_crc(self):
  self.assertIn('wlp_is_game(list[i].titleId,(uint32_t)list[i].appType)',v111)
  self.assertIn('wlp_game_set_hash(game_ids,n,fingerprint)',v111)
  self.assertIn('memcmp(fingerprint,db_last_fingerprint,32)==0',v111)
  self.assertIn('uint32_t master_crc=wlpdb_u32(db_bytes+16)',v111)
  self.assertNotIn('count<=db_best_count',v111)
 def test_immutable_file_and_errors(self):
  self.assertIn('DB_OUTPUT_ROOT',v111)
  self.assertIn('FILE *existing=fopen(final,"rb")',v111)
  self.assertIn('rename(part,final)',v111)
  self.assertIn('db_event("RENAME_FAILED",e,count,games,hash)',v111)
  self.assertIn('db_event("DESTINATION_LOOKUP_FAILED",e,count,n,hash)',v111)
  self.assertIn('db_event("SNAPSHOT_SAVED",0,count,games,hash)',v111)
  self.assertIn('db_event("UNCHANGED_ALREADY_SAVED",0,count,n,hash)',v111)
  self.assertIn('fopen(DB_EVENTS,"ab")',v111)
 def test_failed_writer_must_not_gate_filtering(self):
  self.assertLess(v111.index('db_audit_capture(list,count,cap,&stats);'),v111.index('uint32_t write=0;',v111.index('DECL_FUNCTION(int32_t,MCP_TitleList')))
  self.assertIn('if(db_write_failed)return;',v111)
  self.assertIn('if(db_opted_in)db_load_once();',v111)
if __name__=='__main__':unittest.main()
