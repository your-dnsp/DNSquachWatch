#include "research.h"
#include "field_tools.h"
#include "signatures.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#endif
namespace Research {
namespace {
#if defined(ARDUINO_ARCH_ESP32)
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct Guard { Guard(){portENTER_CRITICAL(&mux);} ~Guard(){portEXIT_CRITICAL(&mux);} };
#else
struct Guard { Guard() {} ~Guard() {} };
#endif
Stats s; Sink sink=nullptr; ReportSink reportSink=nullptr; char message[64]="Ready: optional five-minute research session";
Record queue[12], history[8], pinned; bool pinnedValid=false; uint8_t head=0,count=0,hcount=0,hhead=0; uint32_t nextId=0;
struct Rule { uint16_t id; char kind; DetectionType type; char value[33]; }; // provenance stays in the reviewed input file, not scarce radio RAM
struct Catalog { uint32_t version=BUILTIN_VERSION; uint8_t count=0; Rule rules[12]{}; };
Catalog current, previous, staging; bool havePrevious=false; bool summaryPending=false;
void inc(uint32_t& x){if(x!=UINT32_MAX) ++x;}
bool contains(const char* text,const char* part) {
    if(!text||!part||!*part) return false;
    for(;*text;++text) { size_t j=0; while(part[j] && text[j] && tolower((unsigned char)text[j])==tolower((unsigned char)part[j])) ++j; if(!part[j]) return true; }
    return false;
}
bool hex(const char* p,size_t n){if(strlen(p)!=n)return false;for(size_t i=0;i<n;i++) if(!isxdigit((unsigned char)p[i])) return false; return p[n]==0;}
uint32_t number(const char* p) { if(!p||!*p) return 0; uint32_t n=0; for(;*p;p++){if(*p<'0'||*p>'9'||n>(UINT32_MAX-9)/10)return 0;n=n*10+(*p-'0');}return n; }
void safeNote(char* dst,const char* text){size_t i=0;if(text)for(;i<48&&text[i];i++)dst[i]=(text[i]>=32&&text[i]<=126&&text[i]!='"'&&text[i]!='\\')?text[i]:' ';dst[i]=0;}
void toHex(const uint8_t* p,size_t n,char* out){const char* h="0123456789abcdef";for(size_t i=0;i<n;i++){out[i*2]=h[p[i]>>4];out[i*2+1]=h[p[i]&15];}out[n*2]=0;}
bool enqueue(const Record& r){if(count==12){inc(s.dropped);return false;}queue[(head+count++)%12]=r;return true;}
// Parse the complete AD stream once. Never join evidence across MAC changes or advertisements.
struct Fields { bool axonCompany=false,axonService=false,metaCompany=false,metaService=false,xuntong=false,accessory=false,serial=false; uint8_t ravens=0; char name[33]{}; uint16_t company[8]{},service[16]{}; uint8_t nc=0,ns=0; char uuid128[4][33]{}; uint8_t nu=0; bool malformed=false; };
Fields fields(const uint8_t* p,size_t n){
    Fields f; if(!p && n){f.malformed=true;return f;}
    for(size_t at=0;at<n;){uint8_t len=p[at++];if(!len)break;if(len>n-at){f.malformed=true;break;}uint8_t type=p[at];const uint8_t* d=p+at+1;size_t size=len-1;
        if(type==0xff&&size>=2){uint16_t id=d[0]|(d[1]<<8);if(f.nc<8)f.company[f.nc++]=id;f.axonCompany|=id==0x034d;f.metaCompany|=id==0x0d53;f.xuntong|=id==0x09c8;
            if(id==0x09c8)for(size_t j=2;j+16<=size;j++){if(d[j]!='T'||d[j+1]!='N')continue;bool digits=true;for(size_t k=2;k<16;k++)if(d[j+k]<'0'||d[j+k]>'9')digits=false;if(digits&&(j+16==size||d[j+16]<'0'||d[j+16]>'9'))f.serial=true;}
        }
        if(type==2||type==3||type==0x16){size_t end=type==0x16?(size>=2?2:0):size;if((type!=0x16&&size%2)||(type==0x16&&size<2))f.malformed=true;
            for(size_t i=0;i+1<end;i+=2){uint16_t id=d[i]|(d[i+1]<<8);if(f.ns<16)f.service[f.ns++]=id;f.axonService|=id==0xfc81||id==0xfe6b||id==0xfe6c;f.metaService|=id==0xfd5f;if(id>=0x3100&&id<=0x3500&&(id&255)==0)f.ravens|=1u<<((id-0x3100)/256);}}
        if(type==6||type==7||type==0x21){size_t end=type==0x21?(size>=16?16:0):size;if((type!=0x21&&size%16)||(type==0x21&&size<16))f.malformed=true;
            for(size_t i=0;i+15<end;i+=16){uint8_t rev[16];for(size_t j=0;j<16;j++)rev[j]=d[i+15-j];char value[33];toHex(rev,16,value);f.accessory|=!strcmp(value,"e8ccbb38953246a89fe51814df172e6f");if(f.nu<4)strcpy(f.uuid128[f.nu++],value);}}
        if(type==8||type==9){size_t k=size<32?size:32;for(size_t j=0;j<k;j++)f.name[j]=(d[j]>=32&&d[j]<127)?char(d[j]):' ';f.name[k]=0;}
        at+=len;
    }return f;
}
bool ruleBle(const Rule& r,const Fields& f){
    uint16_t id=(uint16_t)strtoul(r.value,nullptr,16);
    if(r.kind=='C')for(uint8_t i=0;i<f.nc;i++)if(f.company[i]==id)return true;
    if(r.kind=='S')for(uint8_t i=0;i<f.ns;i++)if(f.service[i]==id)return true;
    if(r.kind=='U')for(uint8_t i=0;i<f.nu;i++)if(!strcmp(f.uuid128[i],r.value))return true;
    return r.kind=='N'&&contains(f.name,r.value);
}
}
const char* profileName(Profile p){return p==Profile::BLUETOOTH?"Bluetooth only":p==Profile::WIFI?"WiFi only":"Balanced";}
const char* verdictName(Verdict v){return v==Verdict::VISUAL?"visually_seen_nearby":v==Verdict::SUSPECTED?"suspected":v==Verdict::FALSE_POSITIVE?"false_positive":"unreviewed";}
void setSink(Sink f){sink=f;}
void setReportSink(ReportSink f){reportSink=f;}
bool settled(){Guard g;return !s.active&&!count&&(!summaryPending||s.errors);}
bool active(){Guard g;return s.active;}
Profile profile(){Guard g;return s.profile;}
Stats stats(){Guard g;return s;}
const char* status(){return message;}
uint32_t catalogVersion(){Guard g;return current.version;}
bool start(Profile p,bool raw,uint32_t duration,uint32_t now,uint32_t session,bool cardReady){
    if(active())return false;
    {Guard g;if(count){strcpy(message,"Saving pending records; try again");return false;}}
    if(!cardReady||!sink){snprintf(message,sizeof message,"No mounted microSD: session not started");return false;}
    if((unsigned)p>2||duration<60000||duration>900000)return false;
    if(!sink("","",true)){snprintf(message,sizeof message,"Cannot create research files");return false;}
    Guard g;s=Stats{};s.active=true;s.raw=raw;s.profile=p;s.start=now;s.duration=duration;s.session=session;s.catalog=current.version;
    head=count=hcount=hhead=0;pinnedValid=false;nextId=0;summaryPending=true;s.bytes=256;strcpy(message,"Recording; elapsed time is uptime, not UTC");return true;
}
void stop(const char* why){Guard g;s.active=false;snprintf(message,sizeof message,"%s",why);}
void discard(){Guard g;s.active=false;summaryPending=false;head=count=hhead=hcount=0;memset(queue,0,sizeof queue);memset(history,0,sizeof history);pinned=Record{};pinnedValid=false;}
void coverage(uint32_t delta,bool ble,bool wifi,uint8_t ch){Guard g;if(!s.active)return;if(ble)s.bleMs+=delta;if(wifi){s.wifiMs+=delta;if(ch>=1&&ch<=13){s.channelMs[ch]+=delta;s.channels|=1u<<ch;}}}
void observe(uint8_t radio,const uint8_t* mac,uint8_t addr,int8_t rssi,uint8_t ch,const uint8_t* data,size_t len,uint32_t now,const Match& match){
    if(!mac||(!data&&len)||radio>1)return;
    Guard g;if(!s.active||(radio==0&&s.profile==Profile::WIFI)||(radio==1&&s.profile==Profile::BLUETOOTH))return;
    inc(s.observed);Record r;r.id=++nextId;r.at=now-s.start;r.radio=radio;r.addressType=addr;r.rssi=rssi;r.channel=ch;r.match=match;
    memcpy(r.mac,mac,6);r.original=(uint16_t)(len>65535?65535:len);r.length=(uint8_t)(len>PAYLOAD_CAP?PAYLOAD_CAP:len);
    if(r.length)memcpy(r.payload,data,r.length);enqueue(r);
}
bool recent(uint8_t ix,Record& out){Guard g;if(ix>=hcount)return false;out=history[(hhead+8-1-ix)%8];return true;}
bool select(uint8_t ix,Record& out){Guard g;if(ix>=hcount)return false;out=history[(hhead+8-1-ix)%8];pinned=out;pinnedValid=true;return true;}
bool annotate(uint32_t id,Verdict v,const char* note,uint32_t now){
    Guard g;if(!id||(unsigned)v>3||s.errors)return false;
    for(uint8_t i=0;i<=hcount;i++){const Record& candidate=i<hcount?history[i]:pinned;if((i<hcount||pinnedValid)&&candidate.id==id){Record r=candidate;r.reference=id;r.id=++nextId;r.at=now-s.start;r.verdict=v;r.length=0;r.original=0;safeNote(r.note,note);summaryPending=true;return enqueue(r);}}return false;
}
bool encode(const Record& r,const Stats& s,char* json,size_t jc,char* csv,size_t cc){
    if(r.length>PAYLOAD_CAP||!json||!csv)return false;
    char mac[13],payload[PAYLOAD_CAP*2+1],note[49];safeNote(note,r.note);
    if(s.raw){toHex(r.mac,6,mac);toHex(r.payload,r.length,payload);}else{strcpy(mac,"redacted");payload[0]=0;note[0]=0;}
    // Redacted records deliberately contain no persistent device token, names, notes or payload.
    int a=snprintf(json,jc,"{\"schema\":1,\"session\":%lu,\"id\":%lu,\"reference\":%lu,\"uptime_ms\":%lu,\"time_quality\":\"relative\",\"catalog\":%lu,\"radio\":%u,\"address_type\":%u,\"mac\":\"%s\",\"rssi\":%d,\"channel\":%u,\"type\":%u,\"confidence\":%u,\"evidence\":%u,\"rule\":%u,\"ie_fingerprint\":%lu,\"experimental\":true,\"original_bytes\":%u,\"captured_bytes\":%u,\"payload_hex\":\"%s\",\"verdict\":\"%s\",\"note\":\"%s\"}\n",
        (unsigned long)s.session,(unsigned long)r.id,(unsigned long)r.reference,(unsigned long)r.at,(unsigned long)s.catalog,r.radio,r.addressType,mac,r.rssi,r.channel,(unsigned)r.match.type,(unsigned)r.match.conf,r.match.bits,r.match.rule,(unsigned long)(s.raw?r.match.fingerprint:0),r.original,s.raw?r.length:0,payload,verdictName(r.verdict),note);
    int b=snprintf(csv,cc,"%lu,%lu,%lu,%lu,%lu,%u,%s,%d,%u,%u,%u,%u,%s,%u\n",(unsigned long)s.session,(unsigned long)r.id,(unsigned long)r.reference,(unsigned long)r.at,(unsigned long)s.catalog,r.radio,mac,r.rssi,r.channel,(unsigned)r.match.type,r.match.bits,r.match.rule,verdictName(r.verdict),(unsigned)r.match.conf);
    return a>=0&&size_t(a)<jc&&b>=0&&size_t(b)<cc;
}
void tick(uint32_t now){
    {Guard g;if(s.active){s.elapsed=now-s.start;if(s.elapsed>=s.duration){s.active=false;strcpy(message,"Time limit reached");}}}
    // Persist the observation coverage and losses alongside the evidence.
    Stats finalStats;bool finish=false;
    {Guard g;if(!s.active&&!count&&summaryPending&&!s.errors){finalStats=s;summaryPending=false;finish=true;}}
    if(finish){
        char line[700];int n=snprintf(line,sizeof line,"{\"kind\":\"summary\",\"session\":%lu,\"profile\":%u,\"raw\":%s,\"elapsed_ms\":%lu,\"observed\":%lu,\"saved\":%lu,\"omitted\":%lu,\"write_errors\":%lu,\"ble_enabled_ms\":%lu,\"wifi_enabled_ms\":%lu,\"channel_enabled_ms\":[",
          (unsigned long)finalStats.session,(unsigned)finalStats.profile,finalStats.raw?"true":"false",(unsigned long)finalStats.elapsed,(unsigned long)finalStats.observed,(unsigned long)finalStats.saved,(unsigned long)finalStats.dropped,(unsigned long)finalStats.errors,(unsigned long)finalStats.bleMs,(unsigned long)finalStats.wifiMs);
        for(int i=1;i<=13;i++)n+=snprintf(line+n,sizeof(line)-n,"%s%lu",i==1?"":",",(unsigned long)finalStats.channelMs[i]);
        snprintf(line+n,sizeof(line)-n,"]}\n");
        bool ok=sink&&sink(line,"",false);if(reportSink&&!reportSink(finalStats))ok=false;Guard g;if(ok)s.bytes+=strlen(line);else{inc(s.errors);strcpy(message,"Summary write failed");}return;
    }
    // One bounded write per loop. Stop accepting captures on storage failure.
    Record r;Stats snap;{Guard g;if(!count)return;r=queue[head];head=(head+1)%12;--count;snap=s;}
    char json[900],csv[220];
    if(!encode(r,snap,json,sizeof json,csv,sizeof csv)){Guard g;inc(s.errors);s.active=false;s.dropped+=count+1;count=0;strcpy(message,"Record encoding failed");return;}
    const size_t bytes=strlen(json)+strlen(csv);
    if(snap.bytes+bytes>1024*1024-2048){Guard g;s.active=false;s.dropped+=count+1;count=0;strcpy(message,"1 MiB session limit reached");return;}
    if(!sink||!sink(json,csv,false)){Guard g;inc(s.errors);s.active=false;s.dropped+=count+1;count=0;strcpy(message,"microSD write failed; session stopped");return;}
    Guard g;s.bytes+=bytes;inc(s.saved);if(!r.reference){if((unsigned)r.match.type<19&&(unsigned)r.match.conf<3)inc(s.types[(unsigned)r.match.type][(unsigned)r.match.conf]);history[hhead]=r;hhead=(hhead+1)%8;if(hcount<8)++hcount;}
}
Match matchBle(const uint8_t* p,size_t n){
    Fields f=fields(p,n);Match m;if(f.malformed){m.bits=MALFORMED;return m;}
    bool axName=contains(f.name,"Axon"), glasses=contains(f.name,"Ray-Ban")||contains(f.name,"RayBan")||contains(f.name,"Oakley Meta");
    bool numeric=strlen(f.name)==10;for(size_t i=0;i<strlen(f.name);i++)if(!isdigit((unsigned char)f.name[i]))numeric=false;
    bool penguin=contains(f.name,"Penguin-")||contains(f.name,"FS Ext Battery")||contains(f.name,"FSExtBattery");
    if(f.axonCompany||f.axonService||axName){m.type=DetectionType::AXON;m.label="Axon-equip";m.bits=(f.axonCompany?COMPANY:0)|(f.axonService?SERVICE:0)|(axName?NAME:0);m.rule=100;m.conf=(f.axonCompany&&f.axonService)?Confidence::MED_CONF:Confidence::LOW_CONF;}
    else if(f.accessory||penguin||f.serial||(f.xuntong&&numeric)){m.type=DetectionType::FLOCK;m.label="Flock-acc?";m.bits=ACCESSORY|(f.accessory?SERVICE:0)|((penguin||numeric)?NAME:0)|(f.xuntong?COMPANY:0)|(f.serial?SERIAL_PATTERN:0);m.rule=102;m.conf=(f.accessory&&(penguin||f.xuntong))?Confidence::MED_CONF:Confidence::LOW_CONF;}
    else if(f.ravens){unsigned n=0;for(unsigned i=0;i<5;i++)if(f.ravens&(1u<<i))n++;m.type=DetectionType::RAVEN;m.label="Raven";m.bits=SERVICE;m.rule=104;m.conf=n>=3?Confidence::MED_CONF:Confidence::LOW_CONF;}
    else if(f.metaCompany||f.metaService||glasses){m.type=DetectionType::META;m.label=(glasses||(f.metaCompany&&f.metaService))?"Glasses?":"Meta-radio";m.bits=(f.metaCompany?COMPANY:0)|(f.metaService?SERVICE:0)|(glasses?NAME:0);m.rule=101;m.conf=(f.metaCompany&&f.metaService)?Confidence::MED_CONF:Confidence::LOW_CONF;}
    Guard g;for(uint8_t i=0;i<current.count;i++)if(ruleBle(current.rules[i],f)){m.bits|=IMPORTED;if(m.type==DetectionType::UNKNOWN){m.type=current.rules[i].type;m.rule=current.rules[i].id;m.label="SD-rule?";}break;}return m;
}
Match matchWifi(const uint8_t* mac,const char* ssid){
    Match m;if(!mac)return m;
    m.type=lookupOui(mac,&m.conf);if(m.type!=DetectionType::UNKNOWN){m.bits=OUI;m.label="Vendor-only";}
    bool format=false;if(ssid&&!strncmp(ssid,"Flock-",6)){size_t n=strlen(ssid+6);format=(n==4||n==6)&&hex(ssid+6,n);}
    if(ssid&&(format||!strcmp(ssid,"Flock")||contains(ssid,"test_flck"))){m.type=DetectionType::FLOCK;m.label="Flock-name?";m.conf=Confidence::LOW_CONF;m.bits=SSID;m.rule=103;
        if(mac[0]==0xb4&&mac[1]==0x1e&&mac[2]==0x52){m.bits|=OUI;m.conf=Confidence::MED_CONF;}}
    if(const char* fpv=Field::fpvName(ssid)){m.type=DetectionType::FPV;m.label=fpv;m.conf=Confidence::LOW_CONF;m.bits=SSID;m.rule=105;}
    char oui[7];toHex(mac,3,oui);Guard g;
    for(uint8_t i=0;i<current.count;i++){const Rule& r=current.rules[i];bool match=(r.kind=='O'&&!(mac[0]&3)&&!strcmp(oui,r.value))||(r.kind=='W'&&ssid&&contains(ssid,r.value));
        if(match){m.bits|=IMPORTED;if(m.type==DetectionType::UNKNOWN){m.type=r.type;m.rule=r.id;m.label="SD-rule?";}break;}}return m;
}
Match matchManagement(const uint8_t* frame,size_t length){
    if(!frame||length<24||(frame[0]&0x0c))return Match{};
    const unsigned subtype=frame[0]>>4;
    size_t at=(subtype==8||subtype==5)?36:subtype==4?24:length;
    char ssid[33]{};
    while(at+2<=length){unsigned tag=frame[at++],n=frame[at++];if(n>length-at)break;
        if(tag==0&&n<=32){memcpy(ssid,frame+at,n);ssid[n]=0;break;}at+=n;}
    Match m=matchWifi(frame+10,ssid);
    const uint8_t* rid;size_t ridLen;if(RemoteId::wifiPayload(frame,length,rid,ridLen)){m.type=DetectionType::DRONE;m.conf=Confidence::MED_CONF;m.bits=SERVICE;m.rule=106;m.label="WiFi Remote ID";}
    if(subtype==4&&length<=1024){uint32_t hash=2166136261u;bool complete=true,any=false;size_t i=24;
        while(i+2<=length){uint8_t tag=frame[i++],n=frame[i++];if(n>length-i){complete=false;break;}
            if(tag!=0){any=true;hash=(hash^tag)*16777619u;hash=(hash^n)*16777619u;
                // Vendor IE: keep OUI + vendor type only; avoid serial or session values.
                size_t take=tag==221?(n<4?n:4):n;for(size_t j=0;j<take;j++)hash=(hash^frame[i+j])*16777619u;}
            i+=n;
        }
        if(i!=length)complete=false;
        if(complete&&any){m.fingerprint=hash;char value[9];snprintf(value,sizeof value,"%08lx",(unsigned long)hash);Guard g;
            for(uint8_t j=0;j<current.count;j++)if(current.rules[j].kind=='F'&&!strcmp(current.rules[j].value,value)){
                if(m.type==DetectionType::UNKNOWN){m.type=current.rules[j].type;m.rule=current.rules[j].id;m.label="IE-class?";}
                m.bits|=IMPORTED|FINGERPRINT;break;}}
    }
    return m;
}
bool importPack(const char* data,size_t length){
    if(active()){strcpy(message,"Stop research before changing catalog");return false;}
    if(!data||!length||length>PACK_CAP){strcpy(message,"Pack exceeds 4096-byte limit");return false;}
    staging=Catalog{};size_t at=0;bool header=false;
    while(at<length){char line[256];size_t len=0;while(at<length&&data[at]!='\n'){if(len>=255||data[at]<32||data[at]>126)goto invalid;line[len++]=data[at++];}if(at<length)++at;line[len]=0;if(!len)continue;
        char* part[8];uint8_t np=0;part[np++]=line;for(size_t i=0;i<len;i++)if(line[i]=='|'){line[i]=0;if(np==8)goto invalid;part[np++]=line+i+1;}
        if(!header){if(np!=3||strcmp(part[0],"DNSP-SIG")||strcmp(part[1],"1")||!(staging.version=number(part[2])))goto invalid;header=true;continue;}
        if(np!=7||staging.count==12)goto invalid;
        Rule r{};uint32_t id=number(part[0]),type=number(part[3]);if(id<1000||id>65535||strlen(part[1])!=1||strlen(part[2])>32||!*part[2]||strlen(part[4])>20||!*part[4]||strlen(part[5])>96||strncmp(part[5],"https://",8)||strlen(part[6])!=10)goto invalid;
        for(int j=0;j<10;j++)if(j==4||j==7?part[6][j]!='-':!isdigit((unsigned char)part[6][j]))goto invalid;
        if(type!=(unsigned)DetectionType::FLOCK&&type!=(unsigned)DetectionType::AXON&&type!=(unsigned)DetectionType::META&&type!=(unsigned)DetectionType::ALPR)goto invalid;
        r.kind=part[1][0];if(!strchr("CSUNWOF",r.kind))goto invalid;
        if((r.kind=='C'||r.kind=='S')&&!hex(part[2],4))goto invalid;
        if(r.kind=='F'&&!hex(part[2],8))goto invalid;
        if(r.kind=='U'&&!hex(part[2],32))goto invalid;if(r.kind=='O'&&!hex(part[2],6))goto invalid;
        if(r.kind=='O'&&(strtoul(part[2],nullptr,16)&0x030000))goto invalid;
        r.id=id;r.type=(DetectionType)type;strcpy(r.value,part[2]);if(strchr("CSUOF",r.kind))for(char* p=r.value;*p;p++)*p=tolower((unsigned char)*p);
        // Label/source/date were validated above. Preserve the pack alongside exports for provenance.
        for(uint8_t i=0;i<staging.count;i++)if(staging.rules[i].id==r.id)goto invalid;
        staging.rules[staging.count++]=r;
    }
    if(!header||!staging.count)goto invalid;
    {Guard g;previous=current;havePrevious=true;current=staging;}strcpy(message,"Experimental pack loaded; rollback available");return true;
invalid:strcpy(message,"Invalid pack; current catalog unchanged");return false;
}
bool rollback(){if(active()){strcpy(message,"Stop research before rollback");return false;}Guard g;if(!havePrevious){strcpy(message,"No previous catalog");return false;}current=previous;havePrevious=false;strcpy(message,"Previous catalog restored");return true;}
}
