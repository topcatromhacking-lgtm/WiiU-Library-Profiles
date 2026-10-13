import pathlib
import unittest
SRC=(pathlib.Path(__file__).resolve().parents[1]/"standalone_no_sd_diagnostic"/"source"/"main.c").read_text()
class NoSDIsolation(unittest.TestCase):
    def test_never_mounts_or_reads_sd(self):
        for banned in ('WHBMountSdCard(', 'WHBUnmountSdCard(', 'FSMount(', 'FSGetMountSource(', 'fopen(', 'mkdir(', 'opendir(', 'fwrite(', 'fread(', 'MCP_Install(', 'MCP_Uninstall('):
            self.assertNotIn(banned,SRC)
    def test_manual_stages_only(self):
        main=SRC[SRC.index("int main("):]
        for button in ("VPAD_BUTTON_A","VPAD_BUTTON_X","VPAD_BUTTON_Y","VPAD_BUTTON_B"):
            self.assertIn(button,main)
        self.assertNotIn('MCP_Open(',main)
        self.assertNotIn('MCP_TitleCount(',main)
        self.assertNotIn('MCP_TitleList(',main)
    def test_storage_isolated(self):
        self.assertIn('MCP_TitleCount(',SRC)
        self.assertIn('MCP_TitleList(',SRC)
        self.assertIn('MCP_Open(',SRC)
        self.assertIn('NO SD MOUNT, NO DISK I/O.',SRC)
if __name__=="__main__":unittest.main()
