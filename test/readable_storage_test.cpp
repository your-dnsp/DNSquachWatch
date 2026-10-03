#include "test_util.h"
#include "blackbox.h"
#include "clock.h"
#include "readable_logs.h"
#include "sketchy_rule.h"
#include "location_label.h"
#include "SD.h"
#include <fstream>
#include <sstream>
#include <unistd.h>
namespace Settings{bool autoHistory(){return true;}}
namespace Clock{bool trusted(){return false;}uint32_t nowEpoch(){return 0;}}
namespace SketchyRule{uint8_t count(){return 0;}bool recent(uint8_t,Incident&){return false;}}
static Detection event(unsigned n){Detection d;d.type=DetectionType::AIRTAG;d.vendor="test";d.name[0]=0;d.mac[0]=2;d.mac[4]=n>>8;d.mac[5]=n;d.hits=1;d.rssi=-45;return d;}
static std::string read(const char* leaf){std::ifstream f(TestSD::root+"/DNSP Readable Logs/Current/"+leaf);std::stringstream b;b<<f.rdbuf();return b.str();}
static unsigned lines(const std::string& s){return std::count(s.begin(),s.end(),'\n');}
static bool finish(){unsigned ticks=0;while(ReadableLogs::busy()&&ticks++<400000)ReadableLogs::tick();if(!ReadableLogs::succeeded())fprintf(stderr,"Refresh failed: %s | open=%u peak=%u\n",ReadableLogs::status(),TestSD::openHandles,TestSD::peakHandles);return ReadableLogs::succeeded();}
int main(){char root[]="/tmp/dnsp-history-test-XXXXXX";ck("isolated card",mkdtemp(root)!=nullptr);TestSD::root=root;TestSD::maxHandles=2;setenv("SQUACHSIM_NVS",root,1);LocationLabel::begin();BlackBox::begin();BlackBox::wipe();
 for(unsigned i=0;i<20;i++)BlackBox::noteDetection(event(i),false);
 ck("first refresh starts",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL));ck("first refresh completes",finish());ck("every stored event exported",lines(read("SCAN-HISTORY.txt"))==21);
 auto before=read("SCAN-HISTORY.txt");ck("unchanged refresh succeeds",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL)&&finish());ck("no duplicate events",read("SCAN-HISTORY.txt")==before);
 // Large pre-existing history must not be rescanned for ordinary new records.
 {std::ofstream f(TestSD::root+"/DNSP Readable Logs/Current/SCAN-HISTORY.txt",std::ios::app);for(unsigned i=0;i<100000;i++)f<<"older already exported event without the new ID\n";}
 BlackBox::noteDetection(event(100),false);ck("large history starts",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL));unsigned ticks=0;while(ReadableLogs::busy()&&ticks++<1000)ReadableLogs::tick();ck("large history fast path stays bounded",ReadableLogs::succeeded()&&ticks<1000);
 auto total=lines(read("SCAN-HISTORY.txt"));
 // Stop after the first leaf append, before its cursor commit.
 BlackBox::noteDetection(event(101),false);ck("interrupted refresh starts",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL));
 for(unsigned i=0;i<30&&lines(read("SCAN-HISTORY.txt"))==total;i++)ReadableLogs::tick();ReadableLogs::cancel();ck("incomplete record appended before cancel",lines(read("SCAN-HISTORY.txt"))==total+1);
 ck("retry starts",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL));ck("interrupted record retry completes",finish());ck("retry avoids duplicate partial record",lines(read("SCAN-HISTORY.txt"))==total+1);
 BlackBox::noteDetection(event(102),false);ck("write-failure refresh starts",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL));TestSD::writesLeft=0;while(ReadableLogs::busy())ReadableLogs::tick();ck("write failure never reports completion",!ReadableLogs::succeeded());TestSD::writesLeft=-1;ck("failed write remains retryable",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL)&&finish());
 ck("bounded duplicate read buffer",TestSD::largestRead<=1056);
 {std::ofstream f(TestSD::root+"/squachwatch-session-00000001.log");for(unsigned i=0;i<400;i++)f<<"observation "<<i<<"\n";}
 ck("SD file refresh completes",ReadableLogs::start(ReadableLogs::Mode::REFRESH)&&finish());
 auto raw=read("SD-squachwatch-session-00000001.log.txt");ck("SD text contains every row",lines(raw)==400);
 ck("repeat SD copy completes",ReadableLogs::start(ReadableLogs::Mode::REFRESH)&&finish());ck("SD copy is incremental",read("SD-squachwatch-session-00000001.log.txt")==raw);
 ck("at most two card handles",TestSD::peakHandles<=2);std::filesystem::remove_all(root);return report();}
