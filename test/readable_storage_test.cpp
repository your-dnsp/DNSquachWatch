#include "test_util.h"
#include "blackbox.h"
#include "clock.h"
#include "readable_logs.h"
#include "sketchy_rule.h"
#include "location_label.h"
#include "SD.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unistd.h>
namespace Settings{bool autoHistory(){return true;}}
namespace Clock{bool trusted(){return false;}uint32_t nowEpoch(){return 0;}}
namespace SketchyRule{uint8_t count(){return 0;}bool recent(uint8_t,Incident&){return false;}}
static Detection event(unsigned n){Detection d;d.type=DetectionType::AIRTAG;d.vendor="test";d.name[0]=0;if(n==0){strcpy(d.name,"=bad,name\r");d.vendor="t,est";}d.mac[0]=2;d.mac[4]=n>>8;d.mac[5]=n;d.hits=1;d.rssi=-45;return d;}
static std::string read(const char* leaf){std::ifstream f(TestSD::root+"/DNSP Readable Logs/Current/"+leaf);std::stringstream b;b<<f.rdbuf();return b.str();}
static unsigned lines(const std::string& s){return std::count(s.begin(),s.end(),'\n');}
static bool finish(){unsigned ticks=0;while(ReadableLogs::busy()&&ticks++<400000)ReadableLogs::tick();if(!ReadableLogs::succeeded())fprintf(stderr,"Refresh failed: %s | open=%u peak=%u\n",ReadableLogs::status(),TestSD::openHandles,TestSD::peakHandles);return ReadableLogs::succeeded();}
int main(){char root[]="/tmp/dnsp-history-test-XXXXXX";ck("isolated card",mkdtemp(root)!=nullptr);TestSD::root=root;TestSD::maxHandles=2;setenv("SQUACHSIM_NVS",root,1);LocationLabel::begin();BlackBox::begin();BlackBox::wipe();
 for(unsigned i=0;i<20;i++)BlackBox::noteDetection(event(i),false);
 ck("first refresh starts",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL));ck("first refresh completes",finish());ck("every stored event exported",lines(read("SCAN-HISTORY.txt"))==21);
 ck("readable and CSV history retain simulation",read("SCAN-HISTORY.txt").find("SIMULATED")!=std::string::npos&&read("ALERT-HISTORY-v1.5.3.csv").find("simulated,address_provenance")!=std::string::npos);
 {std::istringstream csv(read("ALERT-HISTORY-v1.5.3.csv"));std::string row;bool valid=true;while(std::getline(csv,row))valid&=std::count(row.begin(),row.end(),',')==13;ck("radio text cannot inject CSV columns or rows",valid&&read("ALERT-HISTORY-v1.5.3.csv").find("?bad.name.")!=std::string::npos);}
 auto before=read("SCAN-HISTORY.txt");ck("unchanged refresh succeeds",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL)&&finish());ck("no duplicate events",read("SCAN-HISTORY.txt")==before);
 SD.remove("/DNSP Readable Logs/Current/ALERT-HISTORY-v1.5.3.csv");SD.remove("/DNSP Readable Logs/Current/.simulation-schema-v153");
 ck("pre-simulation checkpoint backfills versioned CSV",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL)&&finish()&&lines(read("ALERT-HISTORY-v1.5.3.csv"))==21);
 ck("schema migration preserves existing readable entries without duplicates",read("SCAN-HISTORY.txt")==before);
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
 // Reproduce a densely populated flash history, not just a long SD text file.
 BlackBox::wipe();std::filesystem::remove_all(TestSD::root+"/DNSP Readable Logs");
 for(unsigned i=0;i<1800;++i)BlackBox::noteDetection(event(i),false);
 BlackBox::testResetReadStats();ck("full flash-ring backup preparation starts",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL));
 ticks=0;unsigned beforeLocationChecks=TestSD::locationChecks,beforeRenames=TestSD::renames,maxFlashReads=0,maxCardWrites=0;while(ReadableLogs::busy()&&ticks++<40000){unsigned beforeCalls=BlackBox::testReadCalls(),beforeWrites=TestSD::writes;ReadableLogs::tick();maxFlashReads=std::max(maxFlashReads,BlackBox::testReadCalls()-beforeCalls);maxCardWrites=std::max(maxCardWrites,TestSD::writes-beforeWrites);}
 ck("full-ring backup preparation completes",ReadableLogs::succeeded()&&ticks<40000);
 ck("schema marker lookup is bounded per refresh",TestSD::locationChecks-beforeLocationChecks<=1);
 ck("history checkpoint commits avoid file rotations",TestSD::renames==beforeRenames);
 ck("one durable card write per history tick",maxCardWrites<=1);
 ck("full-ring scan work is bounded per tick",maxFlashReads<=8);
 ck("full-ring preparation reads each record twice, not quadratically",BlackBox::testReadCalls()<4000);
 ck("all 1800 readable events preserved",lines(read("SCAN-HISTORY.txt"))==1801);
 ck("full-ring retry creates no duplicates",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL)&&finish()&&lines(read("SCAN-HISTORY.txt"))==1801);
 BlackBox::noteDetection(event(1900),false);
 ReadableLogs::automaticTick(60000,true,true);ck("automatic refresh starts",ReadableLogs::automatic()&&ReadableLogs::busy());
 unsigned token=ReadableLogs::progressToken();ReadableLogs::automaticTick(60001,true,false);
 ck("touch or alert pauses rather than cancels",ReadableLogs::busy()&&ReadableLogs::progressToken()==token);
 ReadableLogs::automaticTick(60250,true,true);ck("background resumes after interaction",ReadableLogs::progressToken()>token);token=ReadableLogs::progressToken();ReadableLogs::automaticTick(60251,true,true);ck("automatic card work leaves frames between steps",ReadableLogs::progressToken()==token);ReadableLogs::automaticTick(UINT32_MAX-10,true,true);token=ReadableLogs::progressToken();ReadableLogs::automaticTick(5,true,true);ck("automatic pacing survives millis rollover",ReadableLogs::progressToken()==token);ReadableLogs::automaticTick(239,true,true);ck("automatic pacing resumes across rollover",ReadableLogs::progressToken()>token);ck("paused refresh completes",finish());
 // Cut at successive passes through record preparation, leaves and checkpoint
 // rotation. Reopening must recover the cursor or the sole pending record.
 BlackBox::wipe();std::filesystem::remove_all(TestSD::root+"/DNSP Readable Logs");
 bool cutsSafe=true;
 for(unsigned cut=1;cut<=20;++cut){BlackBox::noteDetection(event(2000+cut),false);cutsSafe&=ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL);for(unsigned pass=0;pass<cut&&ReadableLogs::busy();++pass)ReadableLogs::tick();ReadableLogs::cancel();cutsSafe&=ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL)&&finish()&&lines(read("SCAN-HISTORY.txt"))==cut+1;}
 ck("interruption at each journal pass preserves exactly one record",cutsSafe);
 BlackBox::noteDetection(event(3000),false);
 ck("timing test refresh starts",ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL));
 SimClock::virtualTime=true;SimClock::nowMs=4294967;TestSD::flushDelayMs=87;bool timedFlush=false,resetBetweenSteps=false;
 for(unsigned i=0;i<100&&ReadableLogs::busy();++i){ReadableLogs::tick();if(!strcmp(ReadableLogs::operation(),"flush")&&ReadableLogs::operationMicros()==87000u)timedFlush=true;else if(timedFlush&&ReadableLogs::operationMicros()==0)resetBetweenSteps=true;}
 TestSD::flushDelayMs=0;SimClock::virtualTime=false;
 ck("slow flush identified across micros rollover",timedFlush);
 ck("operation timings reset between steps",resetBetweenSteps);
 ck("timed refresh completes",finish());
 ck("at most two card handles",TestSD::peakHandles<=2);std::filesystem::remove_all(root);return report();}
