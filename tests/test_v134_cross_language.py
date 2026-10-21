"""C emitter to Python verifier v1.3.4, synthetic source only."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src'))
from v133_active_use_trace import load_trace, verify, SAVE_BYTES
ROOT=Path(__file__).resolve().parents[1]

@unittest.skipUnless(shutil.which('cc'), 'Native C compiler required')
class SnapshotNativeTests(unittest.TestCase):
 def test_native_c_source_classifies_and_python_verifies_snapshot(self):
  with tempfile.TemporaryDirectory() as t:
   d=Path(t)
   binary=d/'v134'
   trace=d/'trace.jsonl'
   cc=subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-pedantic',
     str(ROOT/'tests/test_v134_snapshot_capture.c'),'-o',str(binary)],
     check=True,capture_output=True)
   with trace.open('wb') as f:subprocess.run([str(binary)],check=True,stdout=f)
   events=load_trace(trace)
   self.assertEqual(events[-2]['event'], 'ACTIVE_SNAPSHOT')
   self.assertFalse(any(e['event']=='LAYER_REMOVE' for e in events))
   backup=bytes((i*17+7)%256 for i in range(SAVE_BYTES))
   result=verify(events,backup,'80000001')
   self.assertEqual(result['status'],'ACTIVE_USE_VERIFIED_BY_TRACE')
   self.assertEqual(result['verified_bytes'],SAVE_BYTES)
   other=bytearray(backup);other[-1]^=0x80
   self.assertEqual(verify(events,other,'80000001')['status'],'SOURCE_MISMATCH')
