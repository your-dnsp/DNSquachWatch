#include "card_content.h"
#include <cstdio>
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32)
#include <Arduino.h>
#endif
#if defined(ARDUINO_ARCH_ESP32) || defined(CARD_CONTENT_TEST)
#include <SD.h>
#endif
namespace CardContent {
// Version-specific paths and compiled checks reject incomplete/corrupt content.
// Read once per page, outside display-band rendering; never allocate a whole guide.
static const uint32_t checks[]={0xbbb8bc3fu,0xa26adf2bu,0xa3cb867cu,0xe4e037efu,0x6afa5953u,0x744ad867u,0x500de493u,0x3a400826u,0xe595f9b2u,0x93f06f9du,0x5935c392u,0xe3ce9eedu,0xd4012feau,0x0cbbe577u,0x097724e1u,0xf7a83da2u,0xc9d7eec7u,0xa30ec76du,0xacbb1281u,0x98576f40u,0xb370d21du,0x65da4f41u,0x619c00cbu,0xfa3e0104u,0xa65a91d6u,0x8562cee9u,0xaeb5d4aeu,0xed4ad70au,0x67bfea89u,0x7e0e5345u,0xf21e6061u,0x92ff1aecu,0xe3cd29a2u,0x4c39b208u,0x878adb95u,0x94547c1eu,0x678ab100u,0x304a382cu,0x2d8e372au,0x5162d186u,0x6ae67868u,0xa4944227u,0xc650350bu,0xace395beu,0xb827c0feu,0xd5cb0704u,0x23beaa95u,0x8d26c4d3u,0x639aef4eu,0x5b77bba6u,0x6e98f845u,0x52bf9855u,0x687b8531u,0xe4221a36u,0x8a75fcb4u,0x91295f73u,0x1e22af4fu,0xcc0ffe3eu,0x5579208fu,0xd275d739u,0x555a5808u,0x66827d8fu,0x5d7a150au,0x77e9edf5u,0x633806bau,0x8e1f7279u,0x6652c497u,0x241f6241u,0x385cb8b4u,0x2f3a8f44u,0x8f5452ddu,0x1f172ed4u,0x03082002u,0x0c033414u};

bool verifyRequired(){
#if defined(ARDUINO_ARCH_ESP32) || defined(CARD_CONTENT_TEST)
 auto verify=[](const char* path,size_t size,uint32_t expected){File f=SD.open(path,FILE_READ);if(!f||(size&&f.size()!=size)||(!size&&f.size()>2048)){f.close();return false;}uint8_t b[128];uint32_t hash=2166136261u;size_t left=f.size();while(left){size_t want=left>sizeof b?sizeof b:left;int n=f.read(b,want);if(n!=(int)want){f.close();return false;}for(int i=0;i<n;i++)hash=(hash^b[i])*16777619u;left-=n;
#if defined(ARDUINO_ARCH_ESP32)
 delay(1);
#endif
 }f.close();return hash==expected;};
 for(unsigned i=0;i<guideCount;i++){char path[64];snprintf(path,sizeof path,"/DNSP Content/v1.5/guide-%03u.txt",i);if(!verify(path,0,checks[i]))return false;}
 if(!verify("/DNSP Content/v1.5/glyphs.bin",19757,0x59f7ea02u))return false;
 if(!verify("/DNSP Content/v1.5/translations.bin",22731,0x3d8dae9fu))return false;
 if(!verify("/DNSP Content/v1.5/DNSQUACHWATCH INSTALLATION.txt",10405,0x6c9b6d0fu))return false;
 return true;
#else
 return false;
#endif
}
static uint8_t cached=255;static bool wasMounted=false;
void reset(){cached=255;}
void guide(uint8_t page,bool mounted,Guide& view){
 if(page==cached&&mounted==wasMounted)return;
 cached=page;wasMounted=mounted;
 snprintf(view.title,sizeof view.title,"CARD CONTENT NEEDED");
 snprintf(view.body,sizeof view.body,"Install DNSP Content/v1.5 from the release kit onto microSD. Recovery, scanning and Remington remain built in.");
#if defined(ARDUINO_ARCH_ESP32) || defined(CARD_CONTENT_TEST)
 if(!mounted||page>=guideCount)return;
 char path[64],data[240]{};snprintf(path,sizeof path,"/DNSP Content/v1.5/guide-%03u.txt",page);
 File f=SD.open(path,FILE_READ);if(!f||f.size()>=sizeof data){f.close();return;}
 size_t n=f.size();bool ok=f.read((uint8_t*)data,n)==(int)n;f.close();uint32_t h=2166136261u;
 for(size_t i=0;i<n;i++)h=(h^(uint8_t)data[i])*16777619u;
 char* split=strchr(data,'\n');if(!ok||h!=checks[page]||!split)return;
 *split++=0;snprintf(view.title,sizeof view.title,"%s",data);snprintf(view.body,sizeof view.body,"%s",split);
#endif
 return;
}
}
