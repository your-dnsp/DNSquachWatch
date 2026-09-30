import importlib.util
from pathlib import Path
import unittest
spec=importlib.util.spec_from_file_location('adapter',Path(__file__).resolve().parents[1]/'tools/runtime_display.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Adapter(unittest.TestCase):
 def setUp(self):
  self.source='\n'.join(['spi.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));']*2+['spi.setFrequency(SPI_FREQUENCY);','tft_settings.tft_spi_freq = SPI_FREQUENCY/100000;','spi.setFrequency(SPI_READ_FREQUENCY);','spi.beginTransaction(SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, TFT_SPI_MODE));'])
 def test_all_write_paths(self):
  v=m.patch(self.source);self.assertEqual(v.count('dnsp_display_write_hz()'),5);self.assertNotIn('SPI_FREQUENCY',v)
 def test_read_touch_untouched(self):
  v=m.patch(self.source);self.assertIn('spi.setFrequency(SPI_READ_FREQUENCY);',v);self.assertIn('SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, TFT_SPI_MODE)',v)
 def test_missing_site_rejected(self):
  with self.assertRaises(RuntimeError):m.patch(self.source.replace('spi.setFrequency(SPI_FREQUENCY);',''))
 def test_extra_site_rejected(self):
  with self.assertRaises(RuntimeError):m.patch(self.source+'spi.setFrequency(SPI_FREQUENCY);')
 def test_double_patch_rejected(self):
  with self.assertRaises(RuntimeError):m.patch(m.patch(self.source))
if __name__=='__main__':unittest.main()
