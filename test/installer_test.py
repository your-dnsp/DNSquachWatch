import hashlib,importlib.util,json,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('install',Path(__file__).resolve().parents[1]/'tools/install_dnsp.py');install=importlib.util.module_from_spec(spec);spec.loader.exec_module(install)
class Installer(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.kit=Path(self.tmp.name)
  (self.kit/'manifest.json').write_text(json.dumps({'display':'ST7789','target':'cyd-fast'}))
  names=['bootloader.bin','partitions.bin','boot_app0.bin','firmware.bin']
  for n in names:(self.kit/n).write_bytes(b'checked image')
  (self.kit/'SHA256SUMS').write_text(''.join(hashlib.sha256((self.kit/n).read_bytes()).hexdigest()+'  '+n+'\n' for n in names))
 def tearDown(self):self.tmp.cleanup()
 def test_ordinary_preserves(self):
  with patch.object(install.importlib.util,'find_spec',return_value=True),patch.object(install.subprocess,'run') as run:
   install.flash(self.kit,'/dev/example',False);self.assertNotIn('--erase-all',run.call_args.args[0])
 def test_decline_does_nothing(self):
  with patch('builtins.input',return_value='n'),patch.object(install.subprocess,'run') as run:
   install.flash(self.kit,'/dev/example',False,True);run.assert_not_called()
 def test_erase_requires_confirmation(self):
  with patch('builtins.input',return_value='y'),patch.object(install.importlib.util,'find_spec',return_value=True),patch.object(install.subprocess,'run') as run:
   install.flash(self.kit,'/dev/example',False,True);self.assertIn('--erase-all',run.call_args.args[0]);self.assertTrue(run.call_args.kwargs['check'])
 def test_corrupt_image_cannot_erase(self):
  (self.kit/'firmware.bin').write_bytes(b'corrupt')
  with patch('builtins.input') as ask,patch.object(install.subprocess,'run') as run:
   with self.assertRaises(ValueError):install.flash(self.kit,'/dev/example',False,True)
   ask.assert_not_called();run.assert_not_called()
 def test_eof_is_no(self):
  with patch('builtins.input',side_effect=EOFError),patch.object(install.subprocess,'run') as run:
   install.flash(self.kit,'/dev/example',False,True);run.assert_not_called()
if __name__=='__main__':unittest.main()
