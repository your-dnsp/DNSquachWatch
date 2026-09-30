#include "drone_watch.h"
#include "remote_id.h"
#include "test_util.h"
#include <cstring>
#include <string>
using namespace DroneWatch;
static bool fail=false;
static unsigned writes=0;
static std::string last, records;
static bool sink(const char *p,bool begin){if(fail)return false;writes++;last=p;if(!begin)records+=p;return true;}
int main(){
    uint8_t ad[31]={30,0x16,0xfa,0xff,0x0d,0,2,0x12,'A','B','C'};
    uint8_t mac[6]={2,1,2,3,4,5};
    suite("Payload identity does not require advertised UUID list");
    ck("service data only accepted",inspect(false,ad,sizeof ad)==Verdict::DECODED);
    Detection d{};applyDecodedBle(d,true);
    ck("decoded service data creates drone alert",d.type==DetectionType::DRONE&&d.conf==Confidence::MED_CONF&&d.evidence==MatchEvidence::BLE_REMOTE_ID);
    d={};applyDecodedBle(d,false);ck("no invented detection",d.type==DetectionType::UNKNOWN);
    ad[6]=3;ck("unknown version visible",inspect(false,ad,31)==Verdict::UNSUPPORTED);ad[6]=2;
    uint8_t beacon[71]{};beacon[0]=0x80;memcpy(beacon+10,mac,6);
    beacon[36]=221;beacon[37]=33;memcpy(beacon+38,"\xfa\x0b\xbc\x0d",4);
    beacon[43]=0xf2;beacon[44]=25;beacon[45]=1;memcpy(beacon+46,ad+6,25);
    ck("WiFi pack decoded",inspect(true,beacon,71)==Verdict::DECODED);
    beacon[45]=2;ck("bad pack length visible",inspect(true,beacon,71)==Verdict::MALFORMED);beacon[45]=1;
    beacon[43]=0xf3;ck("future version visible",inspect(true,beacon,71)==Verdict::UNSUPPORTED);beacon[43]=0xf2;
    uint8_t hint[42]{};hint[0]=0x80;hint[36]=0;hint[37]=4;memcpy(hint+38,"RID-",4);
    ck("name only remains clue",inspect(true,hint,42)==Verdict::CLUE);
    suite("Focus timing and bounded channel hold");
    wipe();setFocused(true,100);
    ck("WiFi first",wifiPhase(100)&&wifiPhase(15099));ck("BLE window",!wifiPhase(15100)&&!wifiPhase(20099));ck("repeat",wifiPhase(20100));
    observe(true,mac,beacon,71,-40,6,200);
    ck("hold candidate",preferredChannel(300)==6);
    observe(true,mac,beacon,71,-40,6,2300);
    ck("chatty candidate cannot pin forever",preferredChannel(2300)==0);
    channelResult(6,false);ck("channel failures visible",stats().channelErrors==1&&stats().channel==6);
    suite("Bounded raw capture and IO failures");
    setSink(sink);ck("start pending",startCapture(300)&&stats().capture==Capture::STARTING);
    ck("no overlapping recording",!startCapture(301));tick(309);ck("preparing waits for rendered feedback",stats().capture==Capture::STARTING);captureReady();tick(310);
    ck("recording after header",stats().capture==Capture::RECORDING&&writes==1);
    for(int i=0;i<5;i++)observe(false,mac,ad,31,-50,0,400+i);
    ck("queue overflow counted",stats().dropped==2);
    stopCapture();ck("saving state",!settled()&&stats().capture==Capture::SAVING);
    for(int i=0;i<4;i++)tick(500+i);
    ck("drained before complete",settled()&&stats().saved==3);
    ck("hex record",records.find("1e16faff0d")!=std::string::npos);
    fail=true;startCapture(600);captureReady();tick(601);ck("open failure reported",stats().capture==Capture::ERROR);
    fail=false;startCapture(700);captureReady();tick(701);observe(false,mac,ad,31,-50,0,702);fail=true;tick(703);
    ck("write failure ends capture",stats().capture==Capture::ERROR&&stats().dropped==1);
    fail=false;startCapture(800);captureReady();tick(801);tick(60801);ck("automatic timeout",settled());
    Record record;char out[2304];record.length=1025;record.original=1025;
    ck("invalid record bounded",!formatRecord(record,out,sizeof out));record.length=31;record.original=31;
    ck("tiny destination rejected",!formatRecord(record,out,8));
    suite("No-fix clears previous position");
    RemoteId::Info info;info.haveLoc=true;info.lat=12;
    memset(ad+6,0,25);ad[6]=0x12;
    ck("no-fix updates freshness",RemoteId::merge(ad,31,info,123)&&info.noFix&&!info.haveLoc&&info.at==123);
    suite("Untrusted radio corpus");
    uint8_t data[1200];uint32_t seed=5;
    for(int i=0;i<20000;i++){
        for(auto &b:data){seed=seed*1664525+1013904223;b=seed>>24;}
        size_t n=seed%sizeof data;inspect(true,data,n);inspect(false,data,n);
    }
    ck("20000 bounded malformed inputs",true);
    return report();
}
