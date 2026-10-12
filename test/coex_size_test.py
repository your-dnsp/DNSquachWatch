"""Fail closed before adapting an unknown SDK; never modify the source archive."""
from pathlib import Path
import importlib.util, tempfile, unittest
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('coex',Path(__file__).resolve().parents[1]/'tools/runtime_coex.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Guard(unittest.TestCase):
 def test_unknown_sdk_stops_before_copy_or_patch(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);src=root/'libbt.a';src.write_bytes(b'original SDK');out=root/'local';out.mkdir();(out/'bt.c.obj').write_bytes(b'changed')
   with patch.object(m.subprocess,'run') as run:
    with self.assertRaisesRegex(RuntimeError,'Unreviewed Bluetooth'):m.adapt(src,out,'ar','objcopy')
    self.assertEqual(run.call_count,1)
   self.assertEqual(src.read_bytes(),b'original SDK');self.assertFalse((out/'libbt.a').exists())
if __name__=='__main__':unittest.main()
