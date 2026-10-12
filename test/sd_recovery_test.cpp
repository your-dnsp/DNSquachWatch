#include "sd_log.h"
#include "TFT_eSPI.h"
#include "research.h"
#include "blackbox.h"
#include "clock.h"
#include "location_label.h"
#include "SD.h"
#include "ff.h"
#include <cassert>
#include <cstring>
#include <unistd.h>
TFT_eSPI tft;
namespace Research {void setSink(Sink){} void setReportSink(ReportSink){} bool storageSink(const char*,const char*,bool){return false;} bool storageReport(const Stats&){return false;} void storageWipe(){}}
namespace BlackBox {uint16_t bootNumber(){return 1;}}
namespace Clock {bool trusted(){return false;}uint32_t nowEpoch(){return 0;}}
namespace LocationLabel {const char* text(uint32_t){return "no-label-set";}}
int main(){
 char dir[]="/tmp/dnsp-sd-recovery-XXXXXX";assert(mkdtemp(dir));TestSD::root=dir;
 SdLog log;assert(log.begin()&&log.ready());
 assert(CardTest::clock==1000000);
 assert(BusTest::begins==1&&BusTest::sck==18&&BusTest::mosi==23&&BusTest::miso==19);
 char status[320];log.describe(status,sizeof status);assert(strstr(status,"FAT32"));assert(strstr(status,"unavailable"));assert(FatTest::queries==0);
 FatTest::volume.free_clst=512;log.describe(status,sizeof status);assert(strstr(status,"cached"));assert(FatTest::queries==0);
 assert(log.recoveryTest());assert(CardTest::missingRemoves==0);assert(!SD.exists("/.dnsp-card-test.tmp"));
 CardTest::createOk=false;assert(!log.recoveryTest());assert(strstr(log.recoveryStatus(),"cannot create temporary file"));CardTest::createOk=true;
 TestSD::writesLeft=0;assert(!log.recoveryTest());assert(strstr(log.recoveryStatus(),"write failed"));TestSD::writesLeft=-1;
 assert(log.recoveryRemount()&&log.ready());assert(CardTest::mounts==2&&BusTest::begins==1);
 FatTest::result=3;assert(!log.recoveryRemount()&&!log.ready()&&!CardTest::mounted);assert(strstr(log.recoveryStatus(),"validation"));FatTest::result=FR_OK;
 CardTest::rootOk=false;assert(!log.recoveryRemount()&&!log.ready());CardTest::rootOk=true;
 CardTest::mountOk=false;assert(!log.recoveryRemount()&&!log.ready());CardTest::mountOk=true;
 assert(log.recoveryRemount()&&log.recoveryTest());assert(log.safeEnd()&&!log.ready());assert(TestSD::openHandles==0);
 SimClock::virtualTime=true;SimClock::nowMs=0xfffff000u;
 SdLog delayed;unsigned before=CardTest::mounts;delayed.scheduleBoot(millis());
 delayed.describe(status,sizeof status);assert(strstr(status,"waiting for startup"));
 assert(!delayed.begin()&&!delayed.recoveryTest());assert(CardTest::mounts==before);
 SimClock::nowMs+=9999u;delayed.tick();assert(CardTest::mounts==before&&!delayed.ready());
 SimClock::nowMs+=1u;delayed.tick();assert(delayed.ready()&&CardTest::mounts==before+1);
 delayed.tick();assert(CardTest::mounts==before+1);assert(delayed.safeEnd());
 SdLog failed;failed.scheduleBoot(millis());CardTest::mountOk=false;
 SimClock::nowMs+=10000u;failed.tick();assert(!failed.ready()&&CardTest::mounts==before+2);
 SimClock::nowMs+=20000u;failed.tick();assert(CardTest::mounts==before+2);
 failed.describe(status,sizeof status);assert(strstr(status,"Automatic retries are disabled"));
 CardTest::mountOk=true;assert(failed.recoveryRemount()&&failed.ready());assert(failed.safeEnd());
 SdLog stopped;stopped.scheduleBoot(millis());assert(stopped.safeEnd());before=CardTest::mounts;
 SimClock::nowMs+=10000u;stopped.tick();assert(CardTest::mounts==before);
 SimClock::virtualTime=false;assert(FatTest::queries==0);
 std::filesystem::remove_all(dir);puts("SD recovery: explicit card pins, validation, remount, create/write failure, test cleanup and shutdown PASS");
}
