from pathlib import Path
import runpy, unittest, subprocess, tempfile, shutil
root=Path(__file__).resolve().parents[1]
patch=runpy.run_path(str(root/'tools/runtime_sd.py'))['patch']
def sdk_driver():
 import os
 for base in [root.parent/'platformio',Path(os.environ.get('PLATFORMIO_CORE_DIR',str(Path.home()/'.platformio')))]:
  candidate=base/'packages/framework-arduinoespressif32/libraries/SD/src/sd_diskio.cpp'
  if candidate.exists():return candidate
 return None

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
  driver=sdk_driver()
  if driver is None:self.skipTest('pinned SDK checked in firmware job')
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
 def test_sector_write_responses(self):
  driver=sdk_driver()
  if driver is None:self.skipTest('pinned SDK checked in firmware job')
  compiler=shutil.which('c++')
  if not compiler:self.skipTest('C++ compiler unavailable')
  updated=patch(driver.read_text());start=updated.index('bool sdWriteSector(');end=updated.index('bool sdWriteSectors(',start)
  function=updated[start:end]
  harness=r"""#include <cstdint>
#include <cassert>
struct SerialMock {template<class... A> void printf(const char*,A...) {}} Serial;
struct Card {int type;};static Card card{3};static Card* s_cards[]={&card};
#define CARD_SDHC 3
#define WRITE_BLOCK_SINGLE 24
#define SEND_STATUS 13
static int token, attempts, rejectFirst;static bool ready;static unsigned status;
bool sdSelectCard(uint8_t){return true;}
void sdDeselectCard(uint8_t){}
char sdWriteBytes(uint8_t,const char*,char){++attempts;return attempts==1&&rejectFirst?0x0a:token;}
bool sdWait(uint8_t,unsigned){return ready;}
char sdCommand(uint8_t,char,unsigned int,unsigned int*){return 0;}
char sdTransaction(uint8_t,char,unsigned int,unsigned int* p){*p=status;return 0;}
"""+function+r"""
int main(){char data[512]{};
 for(int response: {0,1,5,10,12,31}){
  token=response;attempts=0;rejectFirst=0;ready=true;status=0;
  assert(sdWriteSector(0,data,123)==(response==5));
  assert(attempts==(response==10?3:1));
 }
 token=5;attempts=0;ready=false;assert(!sdWriteSector(0,data,123));
 ready=true;status=1;assert(!sdWriteSector(0,data,123));
 status=0;rejectFirst=1;attempts=0;assert(sdWriteSector(0,data,123)&&attempts==2);
}
"""
  harness='#include <initializer_list>\n#include <cstddef>\n'+harness
  with tempfile.TemporaryDirectory() as d:
   p=Path(d);(p/'write.cpp').write_text(harness)
   compiled=subprocess.run([compiler,'-std=c++11',str(p/'write.cpp'),'-o',str(p/'write')],capture_output=True,text=True)
   self.assertEqual(compiled.returncode,0,compiled.stderr)
   subprocess.run([str(p/'write')],check=True,capture_output=True)
 def test_pinned_driver(self):
  source=sdk_driver()
  if source is None:self.skipTest('pinned SDK checked in firmware job')
  original=source.read_text();updated=patch(original)
  self.assertEqual(updated.count('WRITE_PERI_REG(SPI_USER_REG(2), SPI_USR_MOSI | SPI_USR_MISO | SPI_DOUTDIN);'),2)
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
