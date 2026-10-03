#include "sketchy_rule.h"
#include "location_label.h"
#include <cstring>
#include <cstdio>
#if defined(ARDUINO_ARCH_ESP32)
#include <Preferences.h>
#include <SD.h>
#endif

namespace SketchyRule {
namespace {
constexpr uint32_t WINDOW_MS=90000;
constexpr uint8_t CAP=8;
Incident ring[CAP]{};
uint8_t head=0,used=0;
bool on=true,pending=false;
Incident pendingIncident{};
uint8_t lastAlpr[6]{},lastDeauth[6]{};
uint32_t lastAt=0,nextId=0,lastRetry=0;

bool isAlpr(DetectionType t){return t==DetectionType::FLOCK||t==DetectionType::AXON||t==DetectionType::ALPR;}
void endpoint(Endpoint& o,const Detection& d){o.type=d.type;memcpy(o.mac,d.mac,6);o.rssi=d.rssi;o.channel=d.channel;o.hits=d.hits;o.at=d.lastSeen;o.locationKey=d.locationKey;snprintf(o.name,sizeof o.name,"%s",d.name);snprintf(o.vendor,sizeof o.vendor,"%s",d.vendor?d.vendor:"");}
bool samePair(const Detection& a,const Detection& d){return !memcmp(lastAlpr,a.mac,6)&&!memcmp(lastDeauth,d.mac,6);}

#if defined(ARDUINO_ARCH_ESP32)
Preferences prefs;
void persist(){prefs.putBool("on",on);prefs.putUChar("head",head);prefs.putUChar("used",used);prefs.putULong("next",nextId);prefs.putBytes("events-v12",ring,sizeof ring);}
void macText(char* out,size_t n,const uint8_t* m){snprintf(out,n,"%02X:%02X:%02X:%02X:%02X:%02X",m[0],m[1],m[2],m[3],m[4],m[5]);}
bool exportIncident(Incident& in){
    if(!SD.cardSize())return false;
    const char* dir="/Sketchy Environment";if(!SD.exists(dir)&&!SD.mkdir(dir))return false;
    char a[24],d[24],path[80];macText(a,sizeof a,in.alpr.mac);macText(d,sizeof d,in.deauth.mac);
    snprintf(path,sizeof path,"%s/incident-%lu.txt",dir,(unsigned long)in.id);
    // FILE_WRITE appends on Arduino SD. Remove an incomplete prior attempt so
    // a retry produces one clean, self-contained incident record.
    if(SD.exists(path))SD.remove(path);
    File f=SD.open(path,FILE_WRITE);if(!f)return false;
    char b[768];snprintf(b,sizeof b,
      "DNSquachWatch Sketchy Environment rule alert\nIncident: %lu\nGap: %lu seconds\n"
      "ALPR clue: %s\nALPR MAC: %s\nALPR name: %s\nALPR vendor: %s\nALPR RSSI: %d dBm\nALPR channel: %u\nALPR uptime ms: %lu\n"
      "ALPR location: %s\nDeauth location: %s\nDeauth MAC: %s\nDeauth RSSI: %d dBm\nDeauth channel: %u\nDeauth frames: %u\nDeauth uptime ms: %lu\n"
      "These observations occurred near the same time. This cautions the user; it does not prove the devices or events are related.\n",
      (unsigned long)in.id,(unsigned long)in.gapSeconds,detectionTypeName(in.alpr.type),a,in.alpr.name,in.alpr.vendor,in.alpr.rssi,in.alpr.channel,(unsigned long)in.alpr.at,
      LocationLabel::text(in.alpr.locationKey),LocationLabel::text(in.deauth.locationKey),d,in.deauth.rssi,in.deauth.channel,in.deauth.hits,(unsigned long)in.deauth.at);
    const size_t n=strlen(b);bool ok=f.write((const uint8_t*)b,n)==n;f.flush();f.close();return ok;
}
#else
void persist(){}
bool exportIncident(Incident&){return false;}
#endif
}

void begin(){
#if defined(ARDUINO_ARCH_ESP32)
    prefs.begin("dnspRules",false);on=prefs.getBool("on",true);head=prefs.getUChar("head",0)%CAP;used=prefs.getUChar("used",0);if(used>CAP)used=0;nextId=prefs.getULong("next",0);if(prefs.getBytesLength("events-v12")==sizeof ring)prefs.getBytes("events-v12",ring,sizeof ring);
    else {
      struct OldEndpoint {DetectionType type;uint8_t mac[6];int8_t rssi;uint8_t channel;uint16_t hits;uint32_t at;char name[20];char vendor[16];};
      struct OldIncident {uint32_t id;OldEndpoint alpr,deauth;uint32_t gapSeconds;bool sdExported;};
      OldIncident old[CAP]{};
      if(prefs.getBytesLength("events")==sizeof old&&prefs.getBytes("events",old,sizeof old)==sizeof old){
       for(unsigned i=0;i<CAP;i++){ring[i].id=old[i].id;ring[i].gapSeconds=old[i].gapSeconds;ring[i].sdExported=old[i].sdExported;
        auto convert=[](Endpoint& dst,const OldEndpoint& src){dst.type=src.type;memcpy(dst.mac,src.mac,6);dst.rssi=src.rssi;dst.channel=src.channel;dst.hits=src.hits;dst.at=src.at;memcpy(dst.name,src.name,20);memcpy(dst.vendor,src.vendor,16);dst.locationKey=0;};
        convert(ring[i].alpr,old[i].alpr);convert(ring[i].deauth,old[i].deauth);
       }
       persist();
      }else used=0;
    }
#endif
}
bool enabled(){return on;}
void toggle(){on=!on;persist();}
void setEnabled(bool v){if(on!=v){on=v;persist();}}
uint8_t count(){return used;}
bool recent(uint8_t i,Incident& out){if(i>=used)return false;out=ring[(head+CAP-1-i)%CAP];return true;}

void tick(const DetectionEngine& engine,uint32_t now){
    // Retry every retained incident that missed its microSD write. One record
    // per pass keeps the main loop responsive while ensuring a later alert
    // cannot replace an older pending export.
    if(now-lastRetry>5000){
        lastRetry=now;
        for(uint8_t i=0;i<used;i++){
            const uint8_t idx=(head+CAP-used+i)%CAP;
            if(ring[idx].id&&!ring[idx].sdExported&&exportIncident(ring[idx])){
                ring[idx].sdExported=true;
                if(pendingIncident.id==ring[idx].id)pendingIncident.sdExported=true;
                persist();
                break;
            }
        }
    }
    if(!on)return;
    const Detection *a=nullptr,*d=nullptr;
    for(uint8_t i=0;i<engine.logCount();i++){
        const Detection* x=engine.logAt(i);if(!x||x->restored||!x->active||now-x->lastSeen>WINDOW_MS)continue;
        if(isAlpr(x->type)&&(!a||x->lastSeen>a->lastSeen))a=x;
        if(x->type==DetectionType::DEAUTH&&(!d||x->lastSeen>d->lastSeen))d=x;
    }
    if(!a||!d)return;uint32_t gap=a->lastSeen>d->lastSeen?a->lastSeen-d->lastSeen:d->lastSeen-a->lastSeen;if(gap>WINDOW_MS)return;
    if(samePair(*a,*d)&&now-lastAt<WINDOW_MS)return;
    Incident in{};in.id=++nextId;endpoint(in.alpr,*a);endpoint(in.deauth,*d);in.gapSeconds=gap/1000;in.sdExported=exportIncident(in);
    ring[head]=in;head=(head+1)%CAP;if(used<CAP)used++;pendingIncident=in;pending=true;memcpy(lastAlpr,a->mac,6);memcpy(lastDeauth,d->mac,6);lastAt=now;persist();
}
bool takeAlert(Incident& out){if(!pending)return false;out=pendingIncident;pending=false;return true;}
void storageWipe(){
#if defined(ARDUINO_ARCH_ESP32)
    const char* dir="/Sketchy Environment";if(SD.cardSize()&&SD.exists(dir)){for(uint16_t pass=0;pass<256;pass++){File q=SD.open(dir);if(!q)break;File f=q.openNextFile();if(!f){q.close();break;}char p[112];const char* n=f.name();snprintf(p,sizeof p,"%s/%s",dir,strrchr(n,'/')?strrchr(n,'/')+1:n);f.close();q.close();SD.remove(p);}SD.rmdir(dir);}prefs.clear();
#endif
    memset(ring,0,sizeof ring);head=used=0;nextId=0;pending=false;pendingIncident=Incident{};
}
}
