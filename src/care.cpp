#include "care.h"
#include "field_tools.h"
#include "settings.h"
#include <Preferences.h>
#include <cstdio>
#include <cstring>
namespace Care {
struct Spec {const char* key;uint32_t def,min,max;bool boolean;};
// Public display/alert preferences only. No passwords, PINs, identifiers,
// transmit consent, update permissions, device mutes or calibration.
static const Spec spec[VALUE_COUNT]={
 {"pal",0,0,5,false},{"bg",10,0,11,false},{"inv",0,0,1,true},{"rgbswap",0,0,1,true},
 {"rotlock",0,0,1,true},{"rot",1,0,3,false},{"bglock",0,0,1,true},{"clkfont",0,0,1,false},
 {"clksize",1,0,2,false},{"clkbg",0,0,6,false},{"bri",255,32,255,false},{"alertsecs",30,15,60,false},
 {"autoquiet",0,0,10,false},{"conf",0,0,2,false},{"boring",0,0,1,true},{"pwrOn",0,0,1,true},
 {"pwrScrnT",2,0,5,false},{"pwrDim",16,0,255,false},{"pwrFps",2,0,4,false},{"pwrIdleT",1,0,4,false},
 {"pwrCpu",0,0,2,false},{"pwrWake",1,0,1,true},{"ltOn",1,0,1,true},{"ltAlert",1,0,1,true},
 {"ltMsg",1,0,1,true},{"ltIdle",1,0,2,false},{"banter",2,0,3,false},{"ltColor",0,0,10,false},
 {"ltBright",2,1,5,false},{"tophat",1,0,1,true},{"tempo",70,50,200,false},{"deskBg",255,0,255,false}
};
uint32_t crc(const void* v,size_t n){uint32_t c=~0u;const uint8_t* p=(const uint8_t*)v;while(n--){c^=*p++;for(int i=0;i<8;i++)c=(c>>1)^(0xedb88320u&-(c&1));}return ~c;}
bool valid(const Snapshot& s){
 for(size_t i=0;i<VALUE_COUNT;i++)if(s.values[i]<spec[i].min||s.values[i]>spec[i].max)return false;
 if(s.values[11]%15|| (s.values[12]!=0&&s.values[12]!=5&&s.values[12]!=10))return false;
 if(s.values[31]!=255&&s.values[31]>11)return false;
 return s.language<7&&s.hebrew<=1&&s.contrast<=1&&s.reduced<=1&&s.left<=1&&s.large<=1&&(s.language!=6||s.hebrew);
}
bool encode(const Snapshot& s,char* out,size_t cap){
 if(!out||!valid(s)||cap<64)return false;size_t at=0;
 auto line=[&](const char* key,uint32_t value){int n=snprintf(out+at,cap-at,"%s=%lu\n",key,(unsigned long)value);if(n<0||size_t(n)>=cap-at)return false;at+=n;return true;};
 if(!line("DNSP_PUBLIC_V1",1))return false;
 for(size_t i=0;i<VALUE_COUNT;i++)if(!line(spec[i].key,s.values[i]))return false;
 const uint8_t fields[]={s.language,s.hebrew,s.contrast,s.reduced,s.left,s.large};
 for(int i=0;i<6;i++){char k[4];snprintf(k,sizeof k,"f%d",i);if(!line(k,fields[i]))return false;}
 uint32_t c=crc(out,at);return line("crc32",c);
}
bool decode(const char* p,size_t n,Snapshot& out){
 if(!p||n>=SETTINGS_CAP)return false;size_t at=0;Snapshot s;
 auto line=[&](const char* key,uint32_t& value){size_t k=strlen(key);if(at+k+2>n||memcmp(p+at,key,k)||p[at+k]!='=')return false;at+=k+1;value=0;unsigned digits=0;while(at<n&&p[at]>='0'&&p[at]<='9'){unsigned d=p[at++]-'0';if(++digits>10||value>(UINT32_MAX-d)/10)return false;value=value*10+d;}return digits&&at<n&&p[at++]=='\n';};
 uint32_t v;if(!line("DNSP_PUBLIC_V1",v)||v!=1)return false;
 for(size_t i=0;i<VALUE_COUNT;i++)if(!line(spec[i].key,s.values[i]))return false;
 uint8_t* fields[]={&s.language,&s.hebrew,&s.contrast,&s.reduced,&s.left,&s.large};
 for(int i=0;i<6;i++){char k[4];snprintf(k,sizeof k,"f%d",i);if(!line(k,v)||v>255)return false;*fields[i]=v;}
 size_t end=at;if(!line("crc32",v)||v!=crc(p,end)||at!=n||!valid(s))return false;out=s;return true;
}
void capture(Snapshot& s){Preferences p;p.begin("settings",true);for(size_t i=0;i<VALUE_COUNT;i++)s.values[i]=spec[i].boolean?p.getBool(spec[i].key,spec[i].def):p.getUChar(spec[i].key,spec[i].def);p.end();
 s.language=Field::config.language;s.hebrew=Field::config.hebrew;s.contrast=Field::config.contrast;s.reduced=Field::config.reduced;s.left=Field::config.left;s.large=Field::config.large;
}
bool apply(const Snapshot& s){
 if(!valid(s))return false;Preferences p;if(!p.begin("settings",false))return false;bool ok=true;
 for(size_t i=0;i<VALUE_COUNT;i++){if(spec[i].boolean){p.putBool(spec[i].key,s.values[i]);ok&=p.getBool(spec[i].key,!s.values[i])==bool(s.values[i]);}else{p.putUChar(spec[i].key,s.values[i]);ok&=p.getUChar(spec[i].key,uint8_t(s.values[i]+1))==s.values[i];}}
 p.end();if(!ok)return false;
 Field::config.language=s.language;Field::config.hebrew=s.hebrew;Field::config.contrast=s.contrast;Field::config.reduced=s.reduced;Field::config.left=s.left;Field::config.large=s.large;Field::save();
 Preferences verify;Field::Config check;if(!verify.begin("dnsp-field",true))return false;
 bool persisted=verify.getBytesLength("v1")==sizeof check&&verify.getBytes("v1",&check,sizeof check)==sizeof check&&!memcmp(&check,&Field::config,sizeof check);verify.end();if(!persisted)return false;
 // Hardware applies panel/rotation/clock state consistently on reboot.
 return true;
}
static const SettingsRow choices[]={SettingsRow::FIELD_TOOLS,SettingsRow::BREAKOUT,SettingsRow::RESEARCH,SettingsRow::SD_STATUS,SettingsRow::DESK_OPEN,SettingsRow::DNSP_GUIDE,SettingsRow::POWER_CONTROL,SettingsRow::ACCESSIBILITY,SettingsRow::LANGUAGE,SettingsRow::DIAGNOSTICS};
static const char* names[]={"FIELD TOOLS","BREAKOUT","RESEARCH LAB","MICROSD STATUS","OPEN DESK","WALKTHROUGH","SHUTDOWN / REBOOT","ACCESSIBILITY","LANGUAGE","DIAGNOSTICS"};
static uint8_t favorites[4]={0,1,3,6};static bool gift=false,due=false;
static void save(){Preferences p;if(p.begin("dnsp-care",false)){p.putBytes("fav",favorites,sizeof favorites);p.putBool("gift",gift);p.end();}}
void begin(){Preferences p;if(p.begin("dnsp-care",true)){uint8_t f[4];if(p.getBytesLength("fav")==4&&p.getBytes("fav",f,4)==4)for(int i=0;i<4;i++)if(f[i]<sizeof choices/sizeof choices[0])favorites[i]=f[i];gift=p.getBool("gift",false);due=gift;p.end();}}
SettingsRow favorite(uint8_t i){return i<4?choices[favorites[i]]:SettingsRow::NONE;}
const char* favoriteName(uint8_t i){return i<4?names[favorites[i]]:"?";}
void cycleFavorite(uint8_t i){if(i>=4)return;favorites[i]=(favorites[i]+1)%(sizeof choices/sizeof choices[0]);save();}
void armGift(bool value){gift=value;if(!value)due=false;save();}bool giftDue(){return due&&gift;}bool giftArmed(){return gift;}void consumeGift(){armGift(false);}
static Health h;static uint32_t last=0,first=0;
void noteLoop(uint32_t now,uint32_t heap,uint32_t block){if(h.loops){uint32_t gap=now-last;if(gap>h.maxGap)h.maxGap=gap;if(gap>250)++h.over250;}else first=now;last=now;++h.loops;if(heap<h.minHeap)h.minHeap=heap;if(block<h.minBlock)h.minBlock=block;}
Health health(){return h;}bool bootReady(uint32_t now){return h.loops>=100&&uint32_t(now-first)>=30000;}
}
