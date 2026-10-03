#include "drone_watch.h"
#include "location_label.h"
#include "remote_id.h"
#include <cstring>
#include <cstdio>
#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <SD.h>
#else
#include <mutex>
#endif
namespace DroneWatch {
namespace {
Stats current;
Record queue[3];
uint8_t head=0,count=0;
bool focus=false, ready=false;
uint32_t focusAt=0, candidateAt=0, holdUntil=0, holdStarted=0;
uint8_t candidateChannel=0;
Sink sink=nullptr;
#if defined(ARDUINO_ARCH_ESP32)
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct Guard { Guard(){portENTER_CRITICAL(&mux);} ~Guard(){portEXIT_CRITICAL(&mux);} };
#else
std::mutex mux;
struct Guard { std::lock_guard<std::mutex> lock{mux}; };
#endif
const char *path="/dnsp-rid-capture.jsonl";
bool storage(const char *line,bool begin) {
#if defined(ARDUINO_ARCH_ESP32)
    if(begin && SD.exists(path)) {
        const char *old="/dnsp-rid-capture.previous.jsonl";
        if(SD.exists(old) && !SD.remove(old)) return false;
        if(!SD.rename(path,old)) return false;
    }
    File f=SD.open(path,FILE_APPEND); if(!f)return false;
    size_t n=strlen(line); bool ok=f.write((const uint8_t*)line,n)==n; f.flush();f.close();return ok;
#else
    (void)line;(void)begin;return false;
#endif
}
Verdict packVerdict(const uint8_t *p,size_t n) {
    if(n<3 || (p[0]>>4)!=15 || p[1]!=25 || !p[2] || p[2]>9 || n!=3u+25u*p[2]) return Verdict::MALFORMED;
    if((p[0]&15)>2)return Verdict::UNSUPPORTED;
    RemoteId::Info r;return RemoteId::mergePack(p,n,r,0)?Verdict::DECODED:Verdict::UNSUPPORTED;
}
}
Verdict inspect(bool wifi,const uint8_t *p,size_t n) {
    if(!p || !n)return Verdict::NONE;
    if(!wifi) {
        for(size_t i=0;i<n;) {
            size_t z=p[i];if(!z)break;
            if(z>n-i-1)return Verdict::NONE;
            if(z>=3 && p[i+1]==0x16 && p[i+2]==0xfa && p[i+3]==0xff) {
                if(z<5 || p[i+4]!=0x0d)return Verdict::UNSUPPORTED;
                if(z!=30)return Verdict::MALFORMED;
                if((p[i+6]&15)>2)return Verdict::UNSUPPORTED;
                RemoteId::Info r;
                return RemoteId::merge(p+i,31,r,0)?Verdict::DECODED:Verdict::UNSUPPORTED;
            }
            i+=z+1;
        }
        return Verdict::NONE;
    }
    if(n<24)return Verdict::NONE;
    const uint8_t *payload=nullptr;size_t size=0;
    if(RemoteId::wifiPayload(p,n,payload,size))return Verdict::DECODED;
    bool clue=false;
    if(p[0]==0x80 && n>=36) {
        for(size_t i=36;i<n;) {
            if(n-i<2)return clue?Verdict::CLUE:Verdict::NONE;
            size_t z=p[i+1];
            if(z>n-i-2)return Verdict::NONE;
            if(p[i]==221 && z>=4 && !memcmp(p+i+2,"\xfa\x0b\xbc\x0d",4)) {
                if((p[1]&0x44)||(p[22]&15)||n>1024)return Verdict::UNSUPPORTED;
                return z>=5?packVerdict(p+i+7,z-5):Verdict::MALFORMED;
            }
            if(p[i]==0 && z>=4 && (!memcmp(p+i+2,"RID-",4)||!memcmp(p+i+2,"DJI",3)))clue=true;
            i+=z+2;
        }
    } else if(p[0]==0xd0 && n>=30 && !memcmp(p+24,"\x04\x09\x50\x6f\x9a\x13",6)) {
        for(size_t i=30;i<n;) {
            if(n-i<3)return Verdict::NONE;
            size_t z=p[i+1]|(size_t(p[i+2])<<8);
            if(z>n-i-3)return Verdict::NONE;
            if(p[i]==3 && z>=6 && !memcmp(p+i+3,"\x88\x69\x19\x9d\x92\x09",6))
                return Verdict::UNSUPPORTED; // known service, unsupported or rejected envelope
            i+=z+3;
        }
    }
    return clue?Verdict::CLUE:Verdict::NONE;
}
const char *verdictText(Verdict v) {
    switch(v) {
    case Verdict::DECODED:return "Remote ID decoded (unverified)";
    case Verdict::CLUE:return "Name clue only; not confirmation";
    case Verdict::UNSUPPORTED:return "RID signature; unsupported form";
    case Verdict::MALFORMED:return "RID signature; invalid length";
    default:return "No Remote ID candidate yet";
    }
}
void observe(bool wifi,const uint8_t *mac,const uint8_t *p,size_t n,int8_t rssi,uint8_t channel,uint32_t now) {
    if(!mac||!p||!n)return;
    Verdict verdict=inspect(wifi,p,n);
    Guard g;
    if(wifi)current.wifi++;else current.ble++;
    if(verdict==Verdict::NONE)return;
    current.last=verdict;
    switch(verdict) {
    case Verdict::DECODED:current.decoded++;break;
    case Verdict::CLUE:current.clues++;break;
    case Verdict::UNSUPPORTED:current.unsupported++;break;
    case Verdict::MALFORMED:current.malformed++;break;
    default:break;
    }
    if(wifi && verdict==Verdict::DECODED && channel>=1 && channel<=13) {
        candidateChannel=channel;candidateAt=now;
        // A fixed hold cannot be perpetually extended by a chatty transmitter.
        if(!holdUntil || now-holdStarted>=5000){holdStarted=now;holdUntil=now+2000;}
    }
    if(current.capture!=Capture::RECORDING)return;
    if(count==3){current.dropped++;return;}
    Record &r=queue[(head+count)%3];
    r.at=now;r.locationKey=LocationLabel::currentKey();r.original=n>65535?65535:n;r.length=n>sizeof r.data?sizeof r.data:n;
    r.wifi=wifi;r.channel=channel;r.rssi=rssi;r.verdict=verdict;memcpy(r.mac,mac,6);memcpy(r.data,p,r.length);
    if(n>r.length)current.truncated++;
    count++;
}
Stats stats(){Guard g;return current;}
void setFocused(bool enabled,uint32_t now){Guard g;focus=enabled;focusAt=now;holdUntil=0;}
bool focused(){Guard g;return focus;}
bool wifiPhase(uint32_t now){Guard g;return (now-focusAt)%20000<15000;}
uint8_t preferredChannel(uint32_t now){Guard g;return candidateChannel && now-candidateAt<2000 && (int32_t)(holdUntil-now)>0?candidateChannel:0;}
void channelResult(uint8_t actual,bool success){Guard g;current.channel=actual;if(!success)current.channelErrors++;}
bool startCapture(uint32_t now){
    Guard g;
    if(current.capture==Capture::STARTING||current.capture==Capture::RECORDING||current.capture==Capture::SAVING)return false;
    ready=false;head=count=0;current.saved=current.dropped=current.truncated=0;current.started=now;current.elapsed=0;
    current.capture=Capture::STARTING;return true;
}
void captureReady(){Guard g;ready=true;}
void stopCapture(){Guard g;if(current.capture==Capture::STARTING)current.capture=Capture::DONE;else if(current.capture==Capture::RECORDING)current.capture=Capture::SAVING;}
bool settled(){auto s=stats().capture;return s==Capture::IDLE||s==Capture::DONE||s==Capture::ERROR;}
void setSink(Sink value){sink=value;}
bool formatRecord(const Record &r,char *out,size_t cap){
    if(!out || r.length>sizeof r.data || r.length>r.original || cap<256u+2u*r.length)return false;
    int n=snprintf(out,cap,"{\"uptime_ms\":%lu,\"location\":\"%s\",\"radio\":\"%s\",\"mac\":\"%02x%02x%02x%02x%02x%02x\",\"rssi\":%d,\"channel\":%u,\"verdict\":%u,\"original_bytes\":%u,\"captured_bytes\":%u,\"hex\":\"",(unsigned long)r.at,LocationLabel::text(r.locationKey),r.wifi?"wifi":"ble",r.mac[0],r.mac[1],r.mac[2],r.mac[3],r.mac[4],r.mac[5],r.rssi,r.channel,unsigned(r.verdict),r.original,r.length);
    if(n<0 || size_t(n)+2*r.length+4>cap)return false;
    static const char hex[]="0123456789abcdef";
    for(size_t i=0;i<r.length;i++){out[n++]=hex[r.data[i]>>4];out[n++]=hex[r.data[i]&15];}
    memcpy(out+n,"\"}\n",4);return true;
}
void tick(uint32_t now){
    auto s=stats();
    if(s.capture==Capture::STARTING){
        {Guard g;if(!ready)return;}
        bool ok=(sink?sink:storage)("{\"schema\":1,\"kind\":\"rid_capture\",\"warning\":\"RAW identifiers and broadcast positions; not authenticated\"}\n",true);
        Guard g;current.capture=ok?Capture::RECORDING:Capture::ERROR;current.started=now;return;
    }
    if(s.capture!=Capture::RECORDING && s.capture!=Capture::SAVING)return;
    Record record;bool have=false, finish=false;
    {
        Guard g;
        if(current.capture==Capture::RECORDING){current.elapsed=now-current.started;
            if(current.elapsed>=60000 || current.saved>=512)current.capture=Capture::SAVING;}
        if(count){record=queue[head];head=(head+1)%3;count--;have=true;}
        else if(current.capture==Capture::SAVING)finish=true;
    }
    if(finish){
        char line[180];auto final=stats();
        snprintf(line,sizeof line,"{\"kind\":\"summary\",\"saved\":%lu,\"dropped\":%lu,\"truncated\":%lu,\"elapsed_ms\":%lu}\n",(unsigned long)final.saved,(unsigned long)final.dropped,(unsigned long)final.truncated,(unsigned long)final.elapsed);
        bool ok=(sink?sink:storage)(line,false);Guard g;current.capture=ok?Capture::DONE:Capture::ERROR;
    }
    if(have){
        char line[2304];bool ok=formatRecord(record,line,sizeof line)&&(sink?sink:storage)(line,false);
        Guard g;if(ok)current.saved++;else{current.capture=Capture::ERROR;current.dropped+=count+1;head=count=0;}
    }
}
void wipe(){
    {Guard g;current=Stats{};head=count=0;focus=false;candidateChannel=0;holdUntil=0;memset(queue,0,sizeof queue);}
#if defined(ARDUINO_ARCH_ESP32)
    SD.remove(path);SD.remove("/dnsp-rid-capture.previous.jsonl");
#endif
}
}
