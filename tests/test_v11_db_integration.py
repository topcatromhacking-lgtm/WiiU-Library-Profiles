import unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1]
BASE=(R/'console_menu_inventory_embedded/src/main.cpp').read_text()
NEW=(R/'console_menu_db_integrated/src/main.cpp').read_text()
class V11DBIntegration(unittest.TestCase):
    def test_original_filter_callback_only_has_opt_in_capture(self):
        anchor='DECL_FUNCTION(int32_t,MCP_TitleList'
        orig=BASE[BASE.index(anchor):]
        revised=NEW[NEW.index(anchor):]
        extra=' /* Read-only database join; no changes to allowlist compaction below. */\n db_audit_capture(list,count,cap,&stats);\n'
        self.assertEqual(revised.replace(extra,''),orig)
    def test_no_new_mcp_mount_or_barista_write(self):
        for forbidden in ('WHBMountSdCard(', 'FSGetMountSource(', 'MCP_Open(', 'MCP_TitleCount(', 'meta.xml', 'fopen(BARISTA','fwrite(BARISTA'):
            self.assertNotIn(forbidden, NEW)
        self.assertEqual(NEW.count('real_MCP_TitleList('),1)
    def test_extra_marker_is_opt_in(self):
        self.assertIn('ENABLE-WLP-V11-DB-READ\\n',NEW)
        self.assertIn('db_opted_in=db_marker_present();',NEW)
        self.assertIn('if(db_opted_in)db_load_once();',NEW)
        self.assertIn('if(!db_opted_in || !stats || !wlp_ready(stats)',NEW)
        self.assertLess(NEW.index('account_ok=true;'),NEW.index('if(db_opted_in)db_load_once();'))
    def test_bounded_binary_crc_validation(self):
        self.assertIn('#define DB_MAX_FILE_BYTES (256u * 1024u)',NEW)
        self.assertIn('#define DB_MAX_RECORDS 4096u',NEW)
        self.assertIn('wlpdb_open(&view,raw,size)',NEW)
        self.assertIn('db_bytes=raw;db_view=view;db_state="DB_READY";',NEW)
    def test_fail_open_and_output_not_barista(self):
        self.assertIn('DB_UNAVAILABLE_FILTER_UNCHANGED',NEW)
        self.assertIn('AUDIT_OK_FILTER_UNCHANGED',NEW)
        self.assertIn('v11-db-audit.csv',NEW)
        self.assertIn('v11-db-status.csv',NEW)
        self.assertIn('rename(part,final)',NEW)
        self.assertIn('if(!wlp_is_game(tid,app))continue;',NEW)
    def test_sources_forked_without_modifying_filter(self):
        for n in ('wlp_filter_core.h','wlp_sha256.h','wlp_inventory_core.h'):
            self.assertEqual((R/'console_menu_inventory_embedded/src'/n).read_bytes(),
                             (R/'console_menu_db_integrated/src'/n).read_bytes())
if __name__=='__main__':unittest.main()
