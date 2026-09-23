#include "research.h"
#include "signatures.h"
#include "test_util.h"
#include <string>
#include <vector>
#include <cstring>
using namespace Research;
static std::string json,csv;static bool fail=false;
static bool sink(const char* j,const char* c,bool reset){if(fail)return false;if(reset){json.clear();csv.clear();}else{json+=j;csv+=c;}return true;}
static std::vector<uint8_t> ad(uint8_t t,std::initializer_list<uint8_t> data){std::vector<uint8_t> v{(uint8_t)(data.size()+1),t};v.insert(v.end(),data);return v;}
static void join(std::vector<uint8_t>& a,const std::vector<uint8_t>& b){a.insert(a.end(),b.begin(),b.end());}
static Match match(const std::vector<uint8_t>& v){return matchBle(v.data(),v.size());}
static const uint8_t mac[]={0xb4,0x1e,0x52,1,2,3};
static void observeOne(uint32_t now=1001){const uint8_t p[]={3,0xff,0x4d,3};observe(0,mac,0,-62,0,p,sizeof p,now,matchBle(p,sizeof p));}
int main(){
 suite("Composite evidence and negative controls");
 auto ax=ad(0xff,{0x4d,3});ck("Axon company is equipment clue, not confirmed camera",match(ax).type==DetectionType::AXON&&match(ax).conf==Confidence::LOW_CONF);
 join(ax,ad(3,{0x81,0xfc}));auto m=match(ax);ck("Axon company and service both retained",m.bits==(COMPANY|SERVICE)&&m.conf==Confidence::MED_CONF);
 auto meta=ad(0xff,{0x53,0x0d});ck("Luxottica alone stays low",match(meta).conf==Confidence::LOW_CONF);join(meta,ad(3,{0x5f,0xfd}));ck("Meta combination supports possible glasses",match(meta).conf==Confidence::MED_CONF&&!strcmp(match(meta).label,"Glasses?"));
 ck("service alone is generic Meta radio",!strcmp(match(ad(3,{0x5f,0xfd})).label,"Meta-radio"));
 ck("shared XUNTONG does not become accessory in research matcher",match(ad(0xff,{0xc8,9})).type==DetectionType::UNKNOWN);
 auto acc=ad(7,{0x6f,0x2e,0x17,0xdf,0x14,0x18,0xe5,0x9f,0xa8,0x46,0x32,0x95,0x38,0xbb,0xcc,0xe8});ck("128-bit Flock accessory byte order",match(acc).bits==(ACCESSORY|SERVICE));
 join(acc,ad(0xff,{0xc8,9}));ck("accessory corroboration",match(acc).conf==Confidence::MED_CONF);
 ck("Nordic DFU alone not camera",match(ad(7,{0x23,0xd1,0xbc,0xea,0x5f,0x78,0x23,0x15,0xde,0xef,0x12,0x12,0x30,0x15,0,0})).type==DetectionType::UNKNOWN);
 auto bad=ax;bad.push_back(20);ck("truncated AD invalidates composite",match(bad).type==DetectionType::UNKNOWN&&(match(bad).bits&MALFORMED));
 ck("odd UUID list rejected",match(ad(3,{0x81,0xfc,0})).bits&MALFORMED);
 ck("Flock name and vendor evidence retained",matchWifi(mac,"Flock-123456").bits==(SSID|OUI));
 uint8_t local[]={0x82,0x6b,0xf2,0,0,0};ck("local MAC is not an OUI",lookupOui(local)==DetectionType::UNKNOWN);ck("local address cannot corroborate manufacturer",!(matchWifi(local,"Flock-123456").bits&OUI));
 Confidence conf=Confidence::HIGH_CONF;uint8_t motorola[]={0,4,0x7d,0,0,0};lookupOui(motorola,&conf);ck("Motorola vendor alone stays low",conf==Confidence::LOW_CONF);
 uint8_t beacon[64]{};beacon[0]=0x80;memcpy(beacon+10,mac,6);beacon[36]=1;beacon[37]=1;beacon[38]=0;beacon[39]=0;beacon[40]=10;memcpy(beacon+41,"Flock-1234",10);ck("SSID need not be first IE",matchManagement(beacon,51).bits==(SSID|OUI));
 ck("ordinary Flock-Guest is not a model-specific rule",matchWifi(mac,"Flock-Guest").bits==OUI);
 ck("generic BLE services are not Raven",match(ad(3,{0x0a,0x18,9,0x18,0x19,0x18})).type==DetectionType::UNKNOWN);
 ck("three custom Raven services remain experimental",match(ad(3,{0,0x31,0,0x32,0,0x33})).type==DetectionType::RAVEN&&match(ad(3,{0,0x31,0,0x32,0,0x33})).conf==Confidence::MED_CONF);
 auto numeric=ad(9,{'1','2','3','4','5','6','7','8','9','0'});ck("numeric name alone not Flock",match(numeric).type==DetectionType::UNKNOWN);join(numeric,ad(0xff,{0xc8,9}));ck("numeric name plus supplier is a weak accessory candidate",match(numeric).type==DetectionType::FLOCK&&match(numeric).conf==Confidence::LOW_CONF);
 auto serial=ad(0xff,{0xc8,9,'T','N','7','2','0','2','3','0','2','2','0','0','0','7','7','1'});ck("supplier TN serial retained",match(serial).bits&SERIAL_PATTERN);
 uint8_t probe[]={0x40,0,0,0,0,0,0,0,0,0,2,1,2,3,4,5,0,0,0,0,0,0,0,0,0,0,1,2,0x82,0x84};auto fp=matchManagement(probe,sizeof probe);ck("probe IE fingerprint generated independently of MAC",fp.fingerprint!=0);probe[15]++;ck("same radio shape across addresses stays same fingerprint",matchManagement(probe,sizeof probe).fingerprint==fp.fingerprint);ck("truncated IE cannot become fingerprint",matchManagement(probe,sizeof probe-1).fingerprint==0);
 suite("Transactional, bounded data-only imports");
 const char* pack="DNSP-SIG|1|42\n1000|S|1234|2|Lab Axon|https://example.org/research|2026-09-22\n";
 ck("valid pack loads",importPack(pack,strlen(pack)));ck("imported service remains experimental low",match(ad(3,{0x34,0x12})).bits&IMPORTED);
 ck("short hex rejected safely",!importPack("DNSP-SIG|1|43\n1000|U|a|2|Lab|https://example.org|2026-09-22\n",strlen("DNSP-SIG|1|43\n1000|U|a|2|Lab|https://example.org|2026-09-22\n")));
 ck("failed import preserves active version",catalogVersion()==42);
 std::string duplicate=std::string(pack)+"1000|C|034d|2|Lab|https://example.org|2026-09-22\n";ck("duplicate rule IDs rejected",!importPack(duplicate.data(),duplicate.size()));
 std::string giant(4097,'A');ck("oversized pack rejected",!importPack(giant.data(),giant.size()));ck("rollback returns builtins",rollback()&&catalogVersion()==BUILTIN_VERSION);ck("rollback is one level",!rollback());
 ck("rolled-back rule no longer matches",match(ad(3,{0x34,0x12})).type==DetectionType::UNKNOWN);
 suite("Recorder limits, exports, notes and storage errors");setSink(sink);
 ck("absent card cannot start",!start(Profile::BALANCED,false,60000,1000,7,false));
 ck("valid session",start(Profile::BALANCED,false,60000,1000,7,true));ck("cannot import mid-session",!importPack(pack,strlen(pack)));
 for(int i=0;i<20;i++)observeOne();ck("bounded queue reports losses",stats().observed==20&&stats().dropped==8);
 for(int i=0;i<12;i++)tick(1002);Record r;ck("recent persisted record available",recent(0,r));
 ck("redacted JSON omits raw MAC and payload",json.find("b41e52010203")==std::string::npos&&json.find("03ff4d03")==std::string::npos&&json.find("redacted")!=std::string::npos);
 ck("annotation refers to saved record",annotate(r.id,Verdict::VISUAL,"Nearby, not same radio",1003));tick(1004);ck("annotation is separate from radio confidence",json.find("visually_seen_nearby")!=std::string::npos&&json.find("Nearby, not same radio")==std::string::npos);
 ck("pin record for notes",select(0,r));for(int i=0;i<9;i++){observeOne(1004);tick(1004);}ck("pinned record survives history rollover",annotate(r.id,Verdict::SUSPECTED,"Control",1004));tick(1004);
 ck("invalid annotation ID refused",!annotate(999,Verdict::VISUAL,"",1003));coverage(100,true,true,6);stop();tick(1005);ck("coverage summary exported",json.find("channel_enabled_ms")!=std::string::npos);
 ck("new raw session",start(Profile::BLUETOOTH,true,60000,1000,8,true));observeOne();observe(1,mac,0,-50,6,nullptr,0,1001);tick(1002);ck("profile excludes other radio",stats().observed==1);ck("raw capture includes exact bytes",json.find("03ff4d03")!=std::string::npos);
 ck("time limit stops recording",(tick(61000),!active()));tick(61001);
 ck("wraparound start",start(Profile::BALANCED,false,60000,UINT32_MAX-100,9,true));tick(60000);ck("wraparound timeout",!active());tick(60001);
 ck("write failure session starts",start(Profile::BALANCED,false,60000,1000,10,true));observeOne();fail=true;tick(1002);ck("write error stops capture",!active()&&stats().errors==1);fail=false;
 discard();ck("discard removes pending and notes",!recent(0,r));
 char j[900],c[220];r=Record{};r.length=255;ck("invalid record length rejected",!encode(r,stats(),j,sizeof j,c,sizeof c));
 r.length=0;ck("too-small export buffer rejected",!encode(r,stats(),j,8,c,sizeof c));
 suite("Storage ceiling and untrusted note encoding");
 ck("start bounded large session",start(Profile::BALANCED,true,900000,1000,11,true));
 for(int i=0;i<5000&&active();i++){observeOne(1001+i);tick(1001+i);}tick(7000);
 ck("one MiB ceiling enforced",!active()&&stats().bytes<=1024*1024&&strstr(status(),"limit"));
 discard();r=Record{};strcpy(r.note,"\"\\\n=FORMULA()");Stats raw;raw.raw=true;
 ck("untrusted note cannot escape JSON string",encode(r,raw,j,sizeof j,c,sizeof c)&&!strstr(j,"\\\n")&&strstr(j,"=FORMULA()")&&strstr(c,"FORMULA")==nullptr);
 suite("Malformed input corpus");
 uint32_t seed=17;uint8_t bytes[160];for(int trial=0;trial<10000;trial++){for(auto& b:bytes){seed=seed*1664525+1013904223;b=seed>>24;}size_t n=seed%sizeof bytes;matchBle(bytes,n);matchManagement(bytes,n);importPack((const char*)bytes,n);}
 ck("ten thousand bounded packets and packs processed",true);
 suite("Readable field report preserves limitations and confidence");
 Stats rs;rs.session=42;rs.saved=7;rs.types[1][0]=6;rs.types[1][1]=1;rs.channels=1<<6;rs.channelMs[6]=1000;
 char readable[3072];ck("bounded report",formatReport(rs,readable,sizeof readable));
 ck("qualitative counts and coverage",strstr(readable,"FLOCK: 6 / 1 / 0")&&strstr(readable,"6=1")&&strstr(readable,"not unique devices"));
 ck("no clearance claim",strstr(readable,"No detection does not mean no camera")!=nullptr);
 ck("truncated output rejected",!formatReport(rs,readable,30));
 return report();
}
