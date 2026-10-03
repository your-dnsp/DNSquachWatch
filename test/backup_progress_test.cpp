#include "test_util.h"
#include "backup_progress.h"
#include "history_id_scan.h"
#include "remmy_gesture.h"
#include <string>
#include <algorithm>
struct Reader{std::string data;size_t at=0,maxRead=0;bool fail=false;bool available(){return at<data.size();}int read(uint8_t* p,size_t n){maxRead=std::max(maxRead,n);if(fail)return -1;n=std::min(n,data.size()-at);memcpy(p,data.data()+at,n);at+=n;return int(n);}};
int main(){
 suite("Backup progress and cooperative history scan");Backup::ProgressWatch w;w.start(0);
 for(unsigned i=1;i<=1200;i++){w.observe(i*1000,i);ck("large backup stays active beyond five minutes",!w.stalled(i*1000));}
 ck("elapsed includes all phases",w.seconds(1200000)==1200);ck("stalled backup expires",w.stalled(1320001));
 w.start(UINT32_MAX-500);w.observe(400,1);ck("progress survives millis rollover",!w.stalled(500));ck("stall comparison survives rollover",w.stalled(120401));
 using S=ReadableLogs::IdScan;const char* id="D-12-345-12345678";
 for(int scenario=0;scenario<6;scenario++){Reader r;r.data=std::string(4*1024*1024,'x');if(scenario==0)r.data.replace(1020,strlen(id),id);if(scenario==1)r.data.replace(r.data.size()-strlen(id),strlen(id),id);if(scenario==2)r.data.replace(0,strlen(id),id);if(scenario==4)r.fail=true;if(scenario==5)r.data.clear();
  S scan;S::Result result=S::WAIT;unsigned ticks=0;while(result==S::WAIT&&ticks++<5000)result=scan.tick(r,id);
  ck("correct complete duplicate search",result==((scenario<3)?S::FOUND:scenario==4?S::ERROR:S::ABSENT));ck("one bounded read per step",r.maxRead<=1024);
 }
 Reader r;r.data="abc";S scan;ck("oversized ID rejected",scan.tick(r,"1234567890123456789012345678901234")==S::ERROR);
 suite("Remmy gesture");Remington::TripleTap taps;ck("first two taps do not summon",!taps.tap(100)&&!taps.tap(350));ck("third tap summons",taps.tap(650));ck("gesture resets after summon",!taps.tap(800));ck("separated taps do not combine",!taps.tap(1600)&&!taps.tap(2400));
 Remington::TripleTap wrap;ck("triple tap survives clock rollover",!wrap.tap(UINT32_MAX-200)&&!wrap.tap(UINT32_MAX-50)&&wrap.tap(150));ck("seven-second star pass",Remington::PASS_MS==7000);
 return report();}
