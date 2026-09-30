#include "scan_profile.h"
#include "detection.h"
#include <Preferences.h>
#include <Arduino.h>

namespace ScanProfile {
namespace {
Profile active=BALANCED, beforeCompare=BALANCED;
uint16_t customCycle=3900;
uint8_t customBle=75, maxMinutes=30;
uint32_t maximumUntil=0;
bool running=false, done=false;
uint8_t compareIx=0;
uint32_t compareAt=0, baseWifi=0,baseBle=0,baseSweeps=0,baseDet=0,baseDrops=0;
Result results[4]{};
constexpr uint32_t COMPARE_MS=45000;

void save(){Preferences p;if(p.begin("dnsp-scan",false)){p.putUChar("profile",active);p.putUInt("wifi",customCycle);p.putUChar("ble",customBle);p.putUChar("maxmin",maxMinutes);p.end();}}
void apply(){setScanWindowBase(bleShare());}
void baseline(uint32_t now,uint32_t wf,uint32_t ba,uint32_t sw,uint32_t de,uint32_t dr){compareAt=now;baseWifi=wf;baseBle=ba;baseSweeps=sw;baseDet=de;baseDrops=dr;results[compareIx].minBlock=UINT32_MAX;}
}
void begin(){Preferences p;if(p.begin("dnsp-scan",true)){active=(Profile)p.getUChar("profile",BALANCED);customCycle=(uint16_t)p.getUInt("wifi",3900);customBle=p.getUChar("ble",75);maxMinutes=p.getUChar("maxmin",30);p.end();}
 if(active> CUSTOM||active==MAXIMUM)active=BALANCED;if(customCycle<2000||customCycle>8000)customCycle=3900;if(customBle<40||customBle>85)customBle=75;if(maxMinutes!=10&&maxMinutes!=30&&maxMinutes!=60&&maxMinutes!=255)maxMinutes=30;maximumUntil=0;save();apply();}
Profile current(){return active;}
const char* name(Profile p){static const char* n[]={"STATIONARY","BALANCED","FAST SWEEP","MAXIMUM","CUSTOM"};return p<=CUSTOM?n[p]:"BALANCED";}
const char* description(Profile p){static const char* d[]={"Longer channel listening for stationary research and intermittent signals.","Tested general-purpose balance of WiFi and Bluetooth reception.","Revisits WiFi channels sooner while walking or changing areas.","Aggressive temporary radio schedule; not best for every signal.","Choose bounded WiFi, Bluetooth and Maximum-duration controls."};return p<=CUSTOM?d[p]:d[1];}
void select(Profile p,uint32_t now){if(p>CUSTOM)return;active=p;maximumUntil=(p==MAXIMUM&&maxMinutes!=255)?now+(uint32_t)maxMinutes*60000u:0;save();apply();}
uint16_t wifiCycleMs(){static const uint16_t cycles[]={6000,3900,2600,2000};return active==CUSTOM?customCycle:cycles[active<=MAXIMUM?active:BALANCED];}
uint8_t bleShare(){static const uint8_t shares[]={70,75,65,80};return active==CUSTOM?customBle:shares[active<=MAXIMUM?active:BALANCED];}
uint16_t customWifiMs(){return customCycle;}uint8_t customBleShare(){return customBle;}
void adjustCustomWifi(int d){int v=(int)customCycle+(d<0?-500:500);if(v<2000)v=8000;if(v>8000)v=2000;customCycle=(uint16_t)v;save();if(active==CUSTOM)apply();}
void adjustCustomBle(int d){int v=(int)customBle+(d<0?-5:5);if(v<40)v=85;if(v>85)v=40;customBle=(uint8_t)v;save();if(active==CUSTOM)apply();}
void setCustomWifi(uint16_t v){if(v<2000)v=2000;if(v>8000)v=8000;v=(uint16_t)(2000+((v-2000+250)/500)*500);customCycle=v;save();if(active==CUSTOM)apply();}
void setCustomBle(uint8_t v){if(v<40)v=40;if(v>85)v=85;v=(uint8_t)(40+((v-40+2)/5)*5);customBle=v;save();if(active==CUSTOM)apply();}
uint8_t maximumMinutes(){return maxMinutes;}
void cycleMaximumMinutes(){maxMinutes=maxMinutes==10?30:maxMinutes==30?60:maxMinutes==60?255:10;save();}
void restore(Profile p,uint16_t w,uint8_t b,uint8_t m){if(p>CUSTOM||p==MAXIMUM)p=BALANCED;if(w>=2000&&w<=8000)customCycle=w;if(b>=40&&b<=85)customBle=b;if(m==10||m==30||m==60||m==255)maxMinutes=m;active=p;maximumUntil=0;save();apply();}
void tick(uint32_t now,uint32_t wf,uint32_t ba,uint32_t sw,uint32_t de,uint32_t dr,uint32_t block){
 if(!running&&active==MAXIMUM&&maximumUntil&&(int32_t)(now-maximumUntil)>=0){select(BALANCED,now);return;}
 if(!running)return;if(block<results[compareIx].minBlock)results[compareIx].minBlock=block;
 if(now-compareAt<COMPARE_MS)return;
 Result& r=results[compareIx];r.wifiFrames=wf-baseWifi;r.bleAdverts=ba-baseBle;r.sweeps=sw-baseSweeps;r.detections=de-baseDet;r.drops=dr-baseDrops;if(r.minBlock==UINT32_MAX)r.minBlock=0;
 if(++compareIx>=4){running=false;done=true;active=beforeCompare;maximumUntil=0;apply();return;}
 active=(Profile)compareIx;apply();baseline(now,wf,ba,sw,de,dr);
}
void startComparison(uint32_t now,uint32_t wf,uint32_t ba,uint32_t sw,uint32_t de,uint32_t dr){beforeCompare=active;running=true;done=false;compareIx=0;for(auto& r:results)r=Result{};active=STATIONARY;maximumUntil=0;apply();baseline(now,wf,ba,sw,de,dr);}
void stopComparison(){if(!running)return;running=false;active=beforeCompare;maximumUntil=0;apply();}
bool comparing(){return running;}bool comparisonDone(){return done;}uint8_t comparisonProfile(){return compareIx;}
uint8_t comparisonPercent(uint32_t now){if(done)return 100;if(!running)return 0;uint32_t total=(uint32_t)compareIx*COMPARE_MS+(now-compareAt);if(total>4*COMPARE_MS)total=4*COMPARE_MS;return (uint8_t)(total*100/(4*COMPARE_MS));}
const Result& result(uint8_t p){return results[p<4?p:0];}
}
