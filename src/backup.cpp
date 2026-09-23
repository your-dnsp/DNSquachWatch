#include "care.h"
#include "ota_core.h"
#include "research.h"
#include "verified_copy.h"
#include <cstdio>
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32)
#include <SD.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_spi_flash.h>
#include <Arduino.h>
#endif
namespace Backup {
static uint8_t selected=0;static bool verified=false;static char message[100]="Copy running firmware + public preferences. Keep power on.";
static Copy job;
const char* status(){return message;}bool busy(){return job.phase==Copy::Phase::WRITE||job.phase==Copy::Phase::VERIFY;}
unsigned percent(){return job.size?unsigned((job.phase==Copy::Phase::VERIFY?50:0)+(uint64_t(job.at)*50/job.size)):0;}
uint8_t slot(){return selected;}void nextSlot(){if(!busy())selected=(selected+1)%10;}
bool verifiedThisBoot(){return verified;}
#if defined(ARDUINO_ARCH_ESP32)
static File file;static const esp_partition_t* part=nullptr;static uint32_t began=0;
static char dir[24],path[56],layoutHash[65];
static void pathFor(const char* leaf){snprintf(path,sizeof path,"%s/%s",dir,leaf);}
static bool writeFile(const char* leaf,const void* p,size_t n){pathFor(leaf);File f=SD.open(path,FILE_WRITE);if(!f)return false;bool ok=f.write((const uint8_t*)p,n)==n;f.flush();f.close();return ok;}
struct Source {bool read(uint32_t at,uint8_t* data,size_t n){return esp_partition_read(part,at,data,n)==ESP_OK;}};
struct Dest {
 bool write(const uint8_t* p,size_t n){return file.write(p,n)==n;}
 bool rewind(uint32_t size){file.flush();file.close();pathFor("firmware.partial");file=SD.open(path,FILE_READ);return file&&file.size()==size;}
 bool read(uint8_t* p,size_t n){return file.read(p,n)==int(n);}
};
void cancel(){if(!busy())return;file.close();job.phase=Copy::Phase::FAILED;strcpy(message,"Incomplete backup retained; no COMPLETE.txt means unusable.");}
bool start(bool card,uint32_t now){
 if(busy())return false;if(!card){strcpy(message,"No mounted microSD.");return false;}
 if(!Research::settled()){strcpy(message,"Wait for research records to finish saving.");return false;}
 if(!Care::bootReady(now)){strcpy(message,"Wait for 30 seconds of responsive operation after boot.");return false;}
 part=esp_ota_get_running_partition();esp_app_desc_t desc;
 if(!part||esp_ota_get_partition_description(part,&desc)!=ESP_OK){strcpy(message,"Running application could not be identified.");return false;}
 if(SD.totalBytes()<SD.usedBytes()+part->size+16384){strcpy(message,"Not enough free space for a verified backup.");return false;}
 // Never overwrite any existing slot, including an interrupted attempt.
 bool freeSlot=false;for(unsigned i=0;i<10;i++){snprintf(dir,sizeof dir,"/dnsp-backup-%u",i);if(!SD.exists(dir)){selected=i;freeSlot=true;break;}}
 if(!freeSlot){strcpy(message,"All 10 slots used. Copy backups to a computer to free a slot.");return false;}
 if(!SD.mkdir(dir)){strcpy(message,"Cannot create backup directory.");return false;}
 Care::Snapshot snap;Care::capture(snap);char text[Care::SETTINGS_CAP];
 DnspHash::Sha256 tableHash;DnspHash::shaInit(tableHash);
 for(unsigned at=0;at<4096;at+=1024){if(spi_flash_read(0x8000+at,text,1024)!=ESP_OK){strcpy(message,"Cannot read partition layout. Backup incomplete.");return false;}DnspHash::shaUpdate(tableHash,(const uint8_t*)text,1024);}
 uint8_t tableDigest[32];DnspHash::shaFinal(tableHash,tableDigest);for(int i=0;i<32;i++)snprintf(layoutHash+2*i,3,"%02x",tableDigest[i]);
 if(!Care::encode(snap,text,sizeof text)||!writeFile("preferences.txt",text,strlen(text))){strcpy(message,"Preferences write failed. Backup incomplete.");return false;}
 pathFor("preferences.txt");File f=SD.open(path,FILE_READ);char readback[Care::SETTINGS_CAP];size_t n=f?f.size():0;
 bool ok=n>0&&n<sizeof readback&&f.read((uint8_t*)readback,n)==int(n);f.close();Care::Snapshot checked;
 if(!ok||!Care::decode(readback,n,checked)){strcpy(message,"Preferences read-back failed. Backup incomplete.");return false;}
 pathFor("firmware.partial");file=SD.open(path,FILE_WRITE);if(!file){strcpy(message,"Cannot create firmware file.");return false;}
 job.start(part->size);began=now;strcpy(message,"Copying running app; scanning continues. Keep power on.");return true;
}
void tick(){
 if(!busy())return;if(uint32_t(millis()-began)>300000){cancel();strcpy(message,"Backup timed out. Incomplete slot retained.");return;}
 Source source;Dest dest;job.tick(source,dest);
 if(job.phase==Copy::Phase::FAILED){file.close();strcpy(message,"Read/write or verification failed. No complete backup.");return;}
 if(job.phase==Copy::Phase::VERIFY)strcpy(message,"Reading back firmware to verify SHA-256.");
 if(job.phase!=Copy::Phase::DONE)return;file.close();
 char from[56];pathFor("firmware.partial");strcpy(from,path);pathFor("firmware.bin");bool ok=SD.rename(from,path);
 char hex[65];for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",job.digest[i]);
 // The app backup deliberately excludes bootloader, NVS, keys, history and
 // partition-table writes. Recovery uses the matching release kit on a PC.
 char manifest[640];int n=snprintf(manifest,sizeof manifest,
  "DNSP_BACKUP_V1\nfirmware=DNSquachWatch v0.7-draft\nbuild=%s\napp_bytes=%lu\nsource_address=0x%lx\nsha256=%s\napp_only=true\npartition_sector_sha256=%s\npreferences=public-v1\nUse the matching board release kit and RECOVERY.md. Hash is integrity, not authenticity.\nNo PINs, WiFi passwords, mesh secrets, history or progression are included.\n",
  OtaCore::buildName(),(unsigned long)part->size,(unsigned long)part->address,hex,layoutHash);
 ok=ok&&n>0&&size_t(n)<sizeof manifest&&writeFile("COMPLETE.txt",manifest,n);
 if(ok){pathFor("COMPLETE.txt");File check=SD.open(path,FILE_READ);char b[640];ok=check&&check.size()==size_t(n)&&check.read((uint8_t*)b,n)==n&&!memcmp(b,manifest,n);check.close();}
 if(ok){verified=true;snprintf(message,sizeof message,"Verified backup in /dnsp-backup-%u. Copy it to a computer.",selected);}else{pathFor("COMPLETE.txt");SD.remove(path);job.phase=Copy::Phase::FAILED;strcpy(message,"Manifest verification failed. Backup incomplete.");}
}
bool exportHealth(bool card,const char* report){
 if(!card||busy()||!report||strlen(report)>1536)return false;
 const char* partial="/dnsp-health.partial",*final="/dnsp-health.txt",*previous="/dnsp-health.previous.txt";
 File f=SD.open(partial,FILE_WRITE);if(!f)return false;size_t n=strlen(report);bool ok=f.write((const uint8_t*)report,n)==n;f.flush();f.close();if(!ok)return false;
 if(SD.exists(final)){if(SD.exists(previous)&&!SD.remove(previous))return false;if(!SD.rename(final,previous))return false;}
 return SD.rename(partial,final);
}
bool restore(bool card){
 if(busy()||!card){strcpy(message,"Stop backup and mount microSD first.");return false;}
 snprintf(dir,sizeof dir,"/dnsp-backup-%u",selected);pathFor("COMPLETE.txt");if(!SD.exists(path)){strcpy(message,"No completed backup in this slot.");return false;}
 pathFor("preferences.txt");File f=SD.open(path,FILE_READ);char b[Care::SETTINGS_CAP];size_t n=f?f.size():0;
 bool ok=n>0&&n<sizeof b&&f.read((uint8_t*)b,n)==int(n);f.close();Care::Snapshot s;
 if(!ok||!Care::decode(b,n,s)){strcpy(message,"Invalid or incompatible preferences. Nothing applied.");return false;}
 ok=Care::apply(s);strcpy(message,ok?"Preferences restored. Reboot now to apply display settings.":"Restore write failed; settings may be partial. Retry after reboot.");return ok;
}
#else
bool start(bool,uint32_t){strcpy(message,"Simulator: physical microSD backup requires the board.");return false;}
void tick(){}void cancel(){}bool exportHealth(bool,const char*){return false;}bool restore(bool){strcpy(message,"Simulator: restore needs microSD on the board.");return false;}
#endif
}
