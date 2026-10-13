import pathlib
import unittest
SRC = (pathlib.Path(__file__).resolve().parents[1] / "standalone_mcp_step_diagnostic" / "source" / "main.c").read_text()
class StepIsolation(unittest.TestCase):
    def test_no_mutation(self):
        for x in ("WUPS_MUST_REPLACE","MCP_Install","MCP_Uninstall","BaristaAccountSaveFile","xml_tag(","read_meta("):
            self.assertNotIn(x,SRC)
    def test_manually_gated_steps(self):
        main=SRC[SRC.index("int main("):]
        for event in ("VPAD_BUTTON_A","VPAD_BUTTON_X","VPAD_BUTTON_Y","VPAD_BUTTON_PLUS","VPAD_BUTTON_B"):
            self.assertIn(event,main)
        self.assertNotIn("MCP_TitleList(",main)
        self.assertNotIn("MCP_TitleCount(",main)
    def test_logged_before_after_mcp(self):
        for stage in ("MCP_OPEN","MCP_TITLECOUNT","MCP_TITLELIST"):
            self.assertIn('stage_log("'+stage+'","BEGIN"',SRC)
            self.assertIn('stage_log("'+stage+'","RETURN"',SRC)
        self.assertIn("fflush(f);",SRC)
    def test_output_and_safety(self):
        self.assertIn("v102-raw-titles.csv",SRC)
        self.assertIn("v102-stages.csv",SRC)
        self.assertNotIn("opendir(",SRC)
        self.assertNotIn("sd:/wiiu/apps",SRC)
if __name__=="__main__":unittest.main()
