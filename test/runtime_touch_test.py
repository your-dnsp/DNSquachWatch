#!/usr/bin/env python3
"""Software-touch transport bit order and pinned-library isolation."""
from pathlib import Path
import runpy,subprocess,tempfile,shutil,unittest
root=Path(__file__).resolve().parents[1]
adapter=runpy.run_path(str(root/'tools/runtime_touch.py'))
class TouchTransport(unittest.TestCase):
 def test_bit_order_and_mode(self):
  compiler=shutil.which('c++')
  if not compiler:self.skipTest('C++ compiler unavailable')
  harness=r'''
#include <stdint.h>
#include <cassert>
#include <vector>
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
static std::vector<int> sent; static uint32_t input=0; static unsigned bits=0; static int touchClock=0,mosi=0;
void pinMode(int,int) {}
void delayMicroseconds(unsigned) {}
void digitalWrite(int pin,int value) { if(pin==32)mosi=value; if(pin==25){if(value && !touchClock)sent.push_back(mosi);touchClock=value;} }
int digitalRead(int pin) { assert(pin==39 && touchClock==HIGH); return (input>>(bits-- -1))&1; }
'''+adapter['PREAMBLE']+r'''
int main(){
 dnsp_touch_begin();assert(touchClock==LOW);
 input=0xa5;bits=8;assert(dnsp_touch_byte(0xb1)==0xa5);assert(bits==0 && touchClock==LOW);
 unsigned encoded=0;for(int v:sent)encoded=(encoded<<1)|v;assert(encoded==0xb1);
 sent.clear();input=0x1234;bits=16;assert(dnsp_touch_word(0xc1)==0x1234);
 encoded=0;for(int v:sent)encoded=(encoded<<1)|v;assert(encoded==0x00c1 && touchClock==LOW && bits==0);
}
'''
  with tempfile.TemporaryDirectory() as d:
   p=Path(d);(p/'test.cpp').write_text(harness)
   subprocess.run([compiler,'-std=c++11',str(p/'test.cpp'),'-o',str(p/'test')],check=True,capture_output=True)
   subprocess.run([str(p/'test')],check=True,capture_output=True)
 def test_pinned_library(self):
  p=root/'.pio/libdeps/cyd-fast/XPT2046_Touchscreen/XPT2046_Touchscreen.cpp'
  if not p.exists():self.skipTest('pinned library not installed')
  s=p.read_text();out=adapter['patch'](s)
  self.assertNotIn('_pspi->begin()',out);self.assertNotIn('_pspi->transfer',out)
  self.assertNotIn('_pspi->beginTransaction',out);self.assertNotIn('_pspi->endTransaction',out)
  for sample in ['data[0]','Z_THRESHOLD','setRotation','isrWake','besttwoavg']:
   if sample in s:self.assertIn(sample,out)
  with self.assertRaises(RuntimeError):adapter['patch'](out)
if __name__=='__main__':unittest.main()
