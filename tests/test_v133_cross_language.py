"""Cross-check captured byte hashes from C against independent Python reference."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from v133_active_use_trace import load_trace, verify, SAVE_BYTES

ROOT=Path(__file__).resolve().parents[1]

@unittest.skipUnless(shutil.which("cc"), "Native C compiler required")
class CaptureRoundtrip(unittest.TestCase):
    def test_c_buffer_capture_python_full_trace(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp)
            binary=d/"capture"
            image=d/"synthetic.dat"
            image.write_bytes(bytes(((i*73+19)%256) for i in range(SAVE_BYTES)))
            transcript=d/"trace.jsonl"
            subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic",
                            str(ROOT/"tests/test_v133_trace_capture.c"),"-o",str(binary)],check=True)
            subprocess.run([str(binary),str(image),str(transcript)],check=True)
            events=load_trace(transcript)
            result=verify(events,image.read_bytes(),"80000001")
            self.assertEqual(result["status"],"ACTIVE_USE_VERIFIED_BY_TRACE")
            self.assertEqual(result["read_events"],32)
            # A byte changed in the supplied backup must invalidate the trace.
            damaged=bytearray(image.read_bytes())
            damaged[8103]^=1
            self.assertEqual(verify(events,damaged,"80000001")["status"],"SOURCE_MISMATCH")
            # A chunk SHA changed in the log must not pass even with correct backup.
            events[4]["sha256"]="0"*64
            self.assertEqual(verify(events,image.read_bytes(),"80000001")["status"],"SOURCE_MISMATCH")

if __name__=="__main__": unittest.main()
