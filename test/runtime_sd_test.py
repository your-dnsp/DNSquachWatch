from pathlib import Path
import runpy, unittest, subprocess, tempfile, shutil
root=Path(__file__).resolve().parents[1]
patch=runpy.run_path(str(root/'tools/runtime_sd.py'))['patch']
class Adapter(unittest.TestCase):
 def test_card_deselected_before_display(self):
  source=(root/'src/main.cpp').read_text();setup=source[source.index('void setup() {'):]
  self.assertLess(setup.index('digitalWrite(5,HIGH);'),setup.index('DuressDevice::boot()'))
  self.assertLess(setup.index('pinMode(5,OUTPUT);'),setup.index('tft.init();'))
 def test_unknown_driver_rejected(self):
  with self.assertRaises(RuntimeError):patch('unreviewed SD driver')
 def test_reset_recovery_exchange(self):
  compiler=shutil.which('c++')
  if not compiler:self.skipTest('C++ compiler unavailable')
  driver=root.parent/'platformio/packages/framework-arduinoespressif32/libraries/SD/src/sd_diskio.cpp'
  updated=patch(driver.read_text())
  begin=updated.index('    AcquireSPI card_locked(card, 100000);')
  end=updated.index('    if (!idle) goto unknown_card;',begin)
  exchange=updated[begin:end].replace('    AcquireSPI card_locked(card, 100000);','')
  harness=r'''#include <cstdint>
#include <cstdio>
#include <cassert>
static uint32_t ticks; static unsigned resets, stops; static int scenario;
uint32_t millis(){return ticks;} void delay(unsigned n){ticks+=n;}
const int HIGH=1, LOW=0, GO_IDLE_STATE=0, STOP_TRANSMISSION=12;
void digitalWrite(int,int){}
struct Bus { uint8_t transfer(uint8_t value){assert(value==0xff); ++ticks; return scenario==3?0:0xff;} } bus;
struct Card {Bus*spi=&bus; int ssPin=5;} storage;
struct Output {template<class... T> void printf(const char*,T...){} } Serial;
void sdDeselectCard(uint8_t){}
bool sdWait(uint8_t,int){ticks+=500; return scenario!=3;}
uint8_t sdCommand(uint8_t,int cmd,int,void*){
 if(cmd==STOP_TRANSMISSION){++stops;return scenario==1?0:0xff;}
 assert(cmd==GO_IDLE_STATE);++resets;
 return scenario==0 || (scenario==1 && stops)?1:0;
}
bool run(){Card*card=&storage;uint8_t pdrv=0;
'''+exchange+r'''return idle;}
int main(){
 for(scenario=0;scenario<4;++scenario){
  ticks=resets=stops=0;bool ok=run();
  assert(ok==(scenario<2));
  assert(stops==(scenario==0?0u:1u));
  assert(resets==(scenario==0?1u:scenario==1?2u:5u));
  assert(ticks<5000);
 }
}
'''
  with tempfile.TemporaryDirectory() as directory:
   src=Path(directory)/'reset.cpp';exe=Path(directory)/'reset';src.write_text(harness)
   subprocess.run([compiler,'-std=c++11',str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
 def test_pinned_driver(self):
  source=root.parent/'platformio/packages/framework-arduinoespressif32/libraries/SD/src/sd_diskio.cpp'
  if not source.exists():self.skipTest('pinned SDK not installed')
  original=source.read_text();updated=patch(original)
  self.assertEqual(updated.count('WRITE_PERI_REG(SPI_USER_REG(3), SPI_USR_MOSI | SPI_USR_MISO | SPI_DOUTDIN);'),2)
  self.assertIn('AcquireSPI card_locked(card, 100000)',updated)
  self.assertIn('attempt<5',updated)
  self.assertIn('uint32_t(millis()-waitStart)<500',updated)
  self.assertLess(updated.index('uint8_t ready=0;'),updated.index('uint8_t response=uint8_t(sdCommand'))
  self.assertIn('selected=%02x',updated)
  self.assertIn('response!=1 && attempt==0',updated)
  self.assertIn('uint8_t stopped=uint8_t(sdCommand(pdrv, STOP_TRANSMISSION, 0, NULL))',updated)
  self.assertIn('count == 0 && stoppedReady',updated)
  self.assertIn('if((++clocks & 63u)==0) delay(1);',updated)
  self.assertIn('if (!idle) goto unknown_card;',updated)
  self.assertNotIn('0x40100000, NULL',updated)
  self.assertIn('APP_OP_COND, 0x40000000, NULL',updated)
  self.assertIn('extern "C" FATFS* dnsp_sd_volume',updated)
  self.assertIn('dnsp_volumes[pdrv]=nullptr;',updated)
  self.assertIn('dnsp_volumes[pdrv]=fs;',updated)
  self.assertIn('fs->free_clst<=fs->n_fatent-2',updated)
  self.assertNotIn('f_getfree',updated[updated.index('extern "C" bool dnsp_sd_cached_space'):updated.index('bool sdWait')])
  # Preserve all sector-write/CRC handling, not merely successful reset paths.
  self.assertEqual(original[original.index('DRESULT ff_sd_write'):original.index('DRESULT ff_sd_ioctl')],updated[updated.index('DRESULT ff_sd_write'):updated.index('DRESULT ff_sd_ioctl')])
  with self.assertRaises(RuntimeError):patch(updated)
if __name__=='__main__':unittest.main()
