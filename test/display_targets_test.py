"""Guard the four shipped panel variants, their pins, and runtime-clock scope."""
import configparser
from pathlib import Path
import re
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Targets(unittest.TestCase):
 def setUp(self):
  self.ini=configparser.ConfigParser(interpolation=None);self.ini.read(ROOT/'platformio.ini')
 def test_four_targets(self):
  for target in ('cyd','cyd-fast','cyd-ili9341','cyd-ili9341-fast'):
   with self.subTest(target=target):
    flags=self.ini['env:'+target]['build_flags']
    self.assertIn('-DDNSP_RUNTIME_DISPLAY=1',flags)
    header='cyd_ili9341_user_setup.h' if 'ili9341' in target else 'cyd_user_setup.h'
    self.assertIn(header,flags)
    self.assertEqual('SPI_FREQUENCY=80000000' in flags,target.endswith('-fast'))
    self.assertIn("'"+target+"'",(ROOT/'extra_script.py').read_text())
 def test_panels_have_same_bus_and_separate_drivers(self):
  a=(ROOT/'include/cyd_user_setup.h').read_text();b=(ROOT/'include/cyd_ili9341_user_setup.h').read_text()
  for name in ('TFT_MISO','TFT_MOSI','TFT_SCLK','TFT_CS','TFT_DC','TFT_RST','TFT_BL','SPI_READ_FREQUENCY'):
   self.assertEqual(re.search(r'^#define\s+'+name+r'\s+(\S+)',a,re.M)[1],re.search(r'^#define\s+'+name+r'\s+(\S+)',b,re.M)[1])
  self.assertRegex(a,r'#define ST7789_DRIVER');self.assertRegex(b,r'#define ILI9341_DRIVER')
  self.assertRegex(a,r'#define TFT_RGB_ORDER\s+TFT_BGR');self.assertRegex(b,r'#define TFT_RGB_ORDER\s+TFT_RGB')
 def test_other_boards_do_not_offer_runtime_toggle(self):
  for name in ('awok','cyd35','phantom'):
   if self.ini.has_section('env:'+name):self.assertNotIn('DNSP_RUNTIME_DISPLAY',self.ini['env:'+name]['build_flags'])
  self.assertIn('defined(DNSP_RUNTIME_DISPLAY)',(ROOT/'src/ui_settings.cpp').read_text())
if __name__=='__main__':unittest.main()
