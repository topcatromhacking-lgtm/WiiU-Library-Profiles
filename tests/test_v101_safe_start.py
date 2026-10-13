import pathlib
import unittest

SRC=(pathlib.Path(__file__).resolve().parents[1]/
     "standalone_inventory_scanner"/"source"/"main.c").read_text()

class SafeStartup(unittest.TestCase):
    def test_scan_only_after_A_button(self):
        main=SRC[SRC.index("int main(int argc,char**argv){"):]
        self.assertIn("if((down & VPAD_BUTTON_A) && !scanned)",main)
        self.assertIn("if(down & VPAD_BUTTON_B)break;",main)
        self.assertGreater(main.index("WHBMountSdCard()"),main.index("if((down & VPAD_BUTTON_A)"))
        self.assertGreater(main.index("collect_titles("),main.index("if((down & VPAD_BUTTON_A)"))
    def test_vpad_pointer_nonnull(self):
        self.assertNotIn("VPADRead(VPAD_CHAN_0,&v,1,NULL)",SRC)
        self.assertIn("VPADRead(VPAD_CHAN_0,&v,1,&err)",SRC)
    def test_menu_unchanged(self):
        for bad in ("WUPS_MUST_REPLACE","MCP_Install","MCP_Uninstall","BaristaAccountSaveFile.dat"):
            self.assertNotIn(bad,SRC)

if __name__=="__main__":
    unittest.main()
