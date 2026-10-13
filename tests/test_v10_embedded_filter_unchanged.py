import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
OLD=(ROOT/"console_menu_profile_switch"/"src"/"main.cpp").read_text()
NEW=(ROOT/"console_menu_inventory_embedded"/"src"/"main.cpp").read_text()
class EmbeddedTelemetrySafety(unittest.TestCase):
    def test_entire_proven_filter_callback_unchanged_except_optional_capture(self):
        suffix="DECL_FUNCTION(int32_t,MCP_TitleList"
        old=OLD[OLD.index(suffix):]
        new=NEW[NEW.index(suffix):]
        extra=' /* Observe ORIGINAL, UNFILTERED TitleList, before in-place compaction. */\n inventory_capture(list,count,cap,&stats);\n'
        self.assertIn(extra,new)
        self.assertEqual(old,new.replace(extra,''))
    def test_no_additional_mcp_open_or_mount(self):
        for banned in ("WHBMountSdCard", "FSGetMountSource", "FSInit(", "MCP_Open(", "MCP_TitleCount(", "meta.xml"):
            self.assertNotIn(banned,NEW)
        self.assertEqual(NEW.count("real_MCP_TitleList("),1)
    def test_inventory_opt_in_and_account_guard(self):
        self.assertIn("ENABLE-WLP-V10-INVENTORY",NEW)
        self.assertIn("inventory_opted_in=inventory_marker_present()",NEW)
        self.assertIn("account_ok=true;",NEW)
        self.assertLess(NEW.index("account_ok=true;"),NEW.index("inventory_opted_in=inventory_marker_present()"))
        self.assertIn("inventory_io_failed=true",NEW)
    def test_writes_only_separate_file(self):
        self.assertIn('v10-inventory.csv.part',NEW)
        self.assertIn('v10-inventory.csv',NEW)
        self.assertIn('rename(INVENTORY_TMP,INVENTORY_CSV)',NEW)
        self.assertIn('WIIU_OR_VC_UNRESOLVED', (ROOT/"console_menu_inventory_embedded"/"src"/"wlp_inventory_core.h").read_text())
        self.assertIn("UNKNOWN",NEW)
    def test_preserve_proven_filter_dependencies(self):
        for path in ("wlp_filter_core.h","wlp_sha256.h"):
            self.assertEqual((ROOT/"console_menu_profile_switch"/"src"/path).read_bytes(),
                             (ROOT/"console_menu_inventory_embedded"/"src"/path).read_bytes())
if __name__=="__main__":unittest.main()
