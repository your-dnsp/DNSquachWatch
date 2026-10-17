"""Verify the hash-guarded scanner adapter without changing the installed SDK."""
import hashlib,importlib.util,os,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('adapter',ROOT/'tools/runtime_timezone_scan.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class AdapterTest(unittest.TestCase):
 def test_pinned_archive_is_scoped_and_source_unchanged(self):
  core=Path(os.environ.get('PLATFORMIO_CORE_DIR',ROOT.parent/'platformio'))
  tool=core/'packages/toolchain-xtensa-esp32'
  src=tool/'xtensa-esp32-elf/lib/no-rtti/libc.a';bins=tool/'bin'
  if not src.exists():self.skipTest('Pinned ESP32 toolchain not installed')
  digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
  before=digest(src)
  with tempfile.TemporaryDirectory() as td:
   dest=Path(td);out=module.adapt(src,dest,bins/'xtensa-esp32-elf-ar',bins/'xtensa-esp32-elf-objcopy')
   self.assertEqual(out,dest/'libc.a');self.assertEqual(before,digest(src))
   symbols=subprocess.check_output([str(bins/'xtensa-esp32-elf-nm'),str(dest/'lib_a-tzset_r.o')],text=True)
   self.assertIn(' U dnspIntegerScanC',symbols);self.assertNotIn(' U siscanf',symbols)
   obj=dest/'lib_a-tzset_r.o';obj.write_bytes(obj.read_bytes()+b'changed')
   bad=dest/'bad.a';subprocess.run([str(bins/'xtensa-esp32-elf-ar'),'rc',str(bad),str(obj)],check=True)
   with self.assertRaisesRegex(RuntimeError,'Unreviewed timezone object'):
    module.adapt(bad,dest/'refused',bins/'xtensa-esp32-elf-ar',bins/'xtensa-esp32-elf-objcopy')
if __name__=='__main__':unittest.main()
