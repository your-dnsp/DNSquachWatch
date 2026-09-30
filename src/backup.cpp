#include "care.h"
#include "ota_core.h"
#include "research.h"
#include "drone_watch.h"
#include "verified_copy.h"
#include "installation_guide.h"
#include "csv_text.h"
#include "user_labels.h"
#include "field_tools.h"
#include "ignore_list.h"
#include "scan_profile.h"
#include "readable_logs.h"
#include "sketchy_rule.h"
#include <cstdio>
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32)
#include <SD.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_spi_flash.h>
#include <Arduino.h>
#include <Preferences.h>
#endif
namespace Backup {
static uint8_t selected=0;static bool verified=false;static char message[100]="Copy running firmware + public preferences. Keep power on.";
static char healthPath[72]="";
static Copy job;static bool preparingLogs=false;
const char* status(){return message;}bool busy(){return preparingLogs||job.phase==Copy::Phase::WRITE||job.phase==Copy::Phase::VERIFY;}
const char* phaseLabel(){return preparingLogs?ReadableLogs::phase():job.phase==Copy::Phase::VERIFY?"VERIFYING":"COPYING";}
unsigned percent(){if(preparingLogs)return ReadableLogs::percent()/2;if(job.phase==Copy::Phase::DONE)return 100;return job.size?50+unsigned((job.phase==Copy::Phase::VERIFY?25:0)+(uint64_t(job.at)*25/job.size)):0;}
uint8_t slot(){return selected;}void nextSlot(){if(!busy())selected=(selected+1)%10;}
bool verifiedThisBoot(){return verified;}
const char* healthExportPath(){return healthPath;}
#if defined(ARDUINO_ARCH_ESP32)
static File file;static const esp_partition_t* part=nullptr;static uint32_t began=0;
static char dir[24],path[56],layoutHash[65];static uint8_t logRows=0;
struct OperationalState{uint32_t magic=0x31504F44u,crc=0;Field::Config field{};uint8_t ignoreN=0,ignore[IgnoreList::MAX][7]{};uint8_t labelN=0,labelMac[16][6]{};UserLabels::Label labels[16]{};uint8_t profile=0,ble=75,maxMin=30,ruleOn=1;uint16_t wifi=3900;uint8_t watchKind=0,watchMac[6]{},huntKind=0,huntMac[6]{};char watchLabel[24]{},huntLabel[24]{};};
struct RestoreJournal{uint32_t magic=0x314A5244u,crc=0;uint8_t slot=0,stage=0;};
struct TargetHandoff{uint32_t magic=0x31544744u,crc=0;uint8_t watchKind=0,watchMac[6]{},huntKind=0,huntMac[6]{};char watchLabel[24]{},huntLabel[24]{};};
static bool validKind(uint8_t k){return k<=(uint8_t)DetectionEngine::WatchKind::WIFI;}
static bool validOperational(const OperationalState& s){
 if(s.magic!=0x31504F44u||s.ignoreN>IgnoreList::MAX||s.labelN>16||s.profile>(uint8_t)ScanProfile::CUSTOM||s.wifi<2000||s.wifi>8000||s.ble<40||s.ble>85||(s.maxMin!=10&&s.maxMin!=30&&s.maxMin!=60&&s.maxMin!=255)||s.ruleOn>1||!validKind(s.watchKind)||!validKind(s.huntKind))return false;
 const Field::Config& f=s.field;if(f.language>=7||f.hebrew>1||f.contrast>1||f.reduced>1||f.left>1||f.large>1||f.quietPrefix>1||f.compositeOnly>1||(f.language==6&&!f.hebrew))return false;
 for(uint8_t i=0;i<4;i++)if(f.bands[i]>5||f.channels[i]>8||f.sensorOn[i]>1||f.muteOn[i]>1)return false;
 for(uint8_t i=0;i<s.ignoreN;i++)if(s.ignore[i][6]>=(uint8_t)DetectionType::COUNT)return false;
 for(uint8_t i=0;i<s.labelN;i++)if(!memchr(s.labels[i].subtag,0,sizeof s.labels[i].subtag)||(s.labels[i].type!=UserLabels::OTHER_TAG&&(s.labels[i].type==0||s.labels[i].type>=(uint8_t)DetectionType::COUNT))||(s.labels[i].type==UserLabels::OTHER_TAG&&!s.labels[i].subtag[0]))return false;
 for(uint8_t i=0;i<s.ignoreN;i++)for(uint8_t j=i+1;j<s.ignoreN;j++)if(!memcmp(s.ignore[i],s.ignore[j],6))return false;
 for(uint8_t i=0;i<s.labelN;i++)for(uint8_t j=i+1;j<s.labelN;j++)if(!memcmp(s.labelMac[i],s.labelMac[j],6))return false;
 if(!memchr(s.watchLabel,0,sizeof s.watchLabel)||!memchr(s.huntLabel,0,sizeof s.huntLabel))return false;
 return true;
}
static bool writeJournal(uint8_t slot,uint8_t stage){RestoreJournal j;j.slot=slot;j.stage=stage;j.crc=0;j.crc=Care::crc(&j,sizeof j);Preferences p;if(!p.begin("dnsp-restore",false))return false;size_t n=p.putBytes("journal",&j,sizeof j);p.end();return n==sizeof j;}
static bool readJournal(RestoreJournal& j){Preferences p;if(!p.begin("dnsp-restore",true))return false;bool ok=p.getBytesLength("journal")==sizeof j&&p.getBytes("journal",&j,sizeof j)==sizeof j;p.end();uint32_t c=j.crc;j.crc=0;return ok&&j.magic==0x314A5244u&&j.slot<10&&j.stage>=1&&j.stage<=3&&Care::crc(&j,sizeof j)==c;}
static void clearJournal(){Preferences p;if(p.begin("dnsp-restore",false)){p.remove("journal");p.end();}}
static bool saveTargets(const OperationalState& s){TargetHandoff h;h.watchKind=s.watchKind;h.huntKind=s.huntKind;memcpy(h.watchMac,s.watchMac,6);memcpy(h.huntMac,s.huntMac,6);memcpy(h.watchLabel,s.watchLabel,sizeof h.watchLabel);memcpy(h.huntLabel,s.huntLabel,sizeof h.huntLabel);h.crc=0;h.crc=Care::crc(&h,sizeof h);Preferences p;if(!p.begin("dnsp-rstr-tgt",false))return false;size_t n=p.putBytes("once",&h,sizeof h);p.end();return n==sizeof h;}
static bool saveFieldVerified(const Field::Config& field){Field::config=field;Field::save();Preferences p;Field::Config check;if(!p.begin("dnsp-field",true))return false;bool ok=p.getBytesLength("v1")==sizeof check&&p.getBytes("v1",&check,sizeof check)==sizeof check&&!memcmp(&check,&field,sizeof field);p.end();return ok;}
static void applyTargets(DetectionEngine& engine,const TargetHandoff& h){engine.clearWatch();engine.clearHunt();if(h.watchKind==(uint8_t)DetectionEngine::WatchKind::BLE)engine.watchBle(h.watchMac,h.watchLabel);else if(h.watchKind==(uint8_t)DetectionEngine::WatchKind::WIFI)engine.watchWifi(h.watchMac,h.watchLabel);if(h.huntKind==(uint8_t)DetectionEngine::WatchKind::BLE)engine.huntBle(h.huntMac,h.huntLabel);else if(h.huntKind==(uint8_t)DetectionEngine::WatchKind::WIFI)engine.huntWifi(h.huntMac,h.huntLabel);}
static void pathFor(const char* leaf){snprintf(path,sizeof path,"%s/%s",dir,leaf);}
static bool writeFile(const char* leaf,const void* p,size_t n){pathFor(leaf);File f=SD.open(path,FILE_WRITE);if(!f)return false;bool ok=f.write((const uint8_t*)p,n)==n;f.flush();f.close();return ok;}
static bool guideChunk(void* context,const uint8_t* p,size_t n){File* f=(File*)context;return f&&f->write(p,n)==n;}
static bool writeInstallationGuide(){pathFor("DNSQUACHWATCH INSTALLATION.txt");File f=SD.open(path,FILE_WRITE);if(!f)return false;bool ok=InstallationGuide::write(guideChunk,&f);f.flush();size_t n=f.size();f.close();return ok&&n==InstallationGuide::size();}
static const char* confName(Confidence c){return c==Confidence::HIGH_CONF?"high":c==Confidence::MED_CONF?"medium":"low";}
static bool writeCurrentLog(const DetectionEngine& engine){
 pathFor("current-log.csv");File out=SD.open(path,FILE_WRITE);if(!out)return false;
 const char* header="row,type,mac,rssi,channel,hits,active,first_seen_ms,last_seen_ms,confidence,vendor,name,user_label,subtag\n";
 bool ok=out.write((const uint8_t*)header,strlen(header))==strlen(header);logRows=0;
 for(uint8_t i=0;ok&&i<engine.logCount();i++){
  const Detection* d=engine.logAt(i);if(!d)continue;
  char vendor[40],name[40],user[28]="",subtag[32]="";safeCsvText(vendor,sizeof vendor,vendorText(*d),strlen(vendorText(*d)));safeCsvText(name,sizeof name,d->name,sizeof d->name);
  UserLabels::Label label{};if(UserLabels::lookup(d->mac,label)){safeCsvText(user,sizeof user,UserLabels::typeName(label.type),strlen(UserLabels::typeName(label.type)));safeCsvText(subtag,sizeof subtag,label.subtag,sizeof label.subtag);}
  char row[384];int n=snprintf(row,sizeof row,"%u,%s,%02X:%02X:%02X:%02X:%02X:%02X,%d,%u,%u,%s,%lu,%lu,%s,%s,%s,%s,%s\n",
    (unsigned)(i+1),detectionTypeName(d->type),d->mac[0],d->mac[1],d->mac[2],d->mac[3],d->mac[4],d->mac[5],(int)d->rssi,(unsigned)d->channel,(unsigned)d->hits,d->active?"yes":"no",
    (unsigned long)d->firstSeen,(unsigned long)d->lastSeen,confName(d->conf),vendor,name,user,subtag);
  ok=n>0&&size_t(n)<sizeof row&&out.write((const uint8_t*)row,n)==size_t(n);if(ok)logRows++;
 }
 out.flush();out.close();if(!ok)return false;
 pathFor("current-log.csv");File check=SD.open(path,FILE_READ);ok=check&&check.size()>=strlen(header);check.close();return ok;
}
static bool writeOperational(const DetectionEngine* engine){OperationalState s;s.field=Field::config;s.ignoreN=IgnoreList::count();for(uint8_t i=0;i<s.ignoreN;i++){const uint8_t*m=IgnoreList::macAt(i);if(m)memcpy(s.ignore[i],m,6);s.ignore[i][6]=(uint8_t)IgnoreList::typeAt(i);}s.labelN=UserLabels::count();for(uint8_t i=0;i<s.labelN;i++)UserLabels::at(i,s.labelMac[i],s.labels[i]);s.profile=(uint8_t)ScanProfile::current();s.wifi=ScanProfile::customWifiMs();s.ble=ScanProfile::customBleShare();s.maxMin=ScanProfile::maximumMinutes();s.ruleOn=SketchyRule::enabled();if(engine){s.watchKind=(uint8_t)engine->watchKind();s.huntKind=(uint8_t)engine->huntKind();if(s.watchKind){memcpy(s.watchMac,engine->watchMac(),6);snprintf(s.watchLabel,sizeof s.watchLabel,"%s",engine->watchLabel());}if(s.huntKind){memcpy(s.huntMac,engine->huntMac(),6);snprintf(s.huntLabel,sizeof s.huntLabel,"%s",engine->huntLabel());}}s.crc=0;s.crc=Care::crc(&s,sizeof s);return writeFile("operational-state.bin",&s,sizeof s);}
static bool copyReadable(const char* leaf){char from[112];snprintf(from,sizeof from,"/DNSP Readable Logs/Current/%s",leaf);File a=SD.open(from,FILE_READ);if(!a)return true;pathFor(leaf);File b=SD.open(path,FILE_WRITE);if(!b){a.close();return false;}uint8_t buf[512];bool ok=true;while(a.available()){int n=a.read(buf,sizeof buf);if(n<=0||b.write(buf,n)!=(size_t)n){ok=false;break;}}b.flush();a.close();b.close();return ok;}
static bool copyReadableSet(){const char* n[]={"ALL-ALERTS.txt","ALERT-HISTORY.csv","SCAN-HISTORY.txt","SYSTEM-HISTORY.txt","SKETCHY-ENVIRONMENT.txt","EXPORT-SUMMARY.txt"};for(const char*x:n)if(!copyReadable(x))return false;return true;}
struct Source {bool read(uint32_t at,uint8_t* data,size_t n){return esp_partition_read(part,at,data,n)==ESP_OK;}};
struct Dest {
 bool write(const uint8_t* p,size_t n){return file.write(p,n)==n;}
 bool rewind(uint32_t size){file.flush();file.close();pathFor("firmware.partial");file=SD.open(path,FILE_READ);return file&&file.size()==size;}
 bool read(uint8_t* p,size_t n){return file.read(p,n)==int(n);}
};
void cancel(){if(!busy())return;if(preparingLogs){ReadableLogs::cancel();preparingLogs=false;}file.close();job.phase=Copy::Phase::FAILED;strcpy(message,"Incomplete backup retained; no COMPLETE.txt means unusable.");}
bool start(bool card,uint32_t now,const DetectionEngine* engine){
 if(busy())return false;if(!card){strcpy(message,"No mounted microSD.");return false;}
 if(!Research::settled() || !DroneWatch::settled()){strcpy(message,"Wait for research records to finish saving.");return false;}
 if(!Care::bootReady(now)){strcpy(message,"Wait for 30 seconds of responsive operation after boot.");return false;}
 part=esp_ota_get_running_partition();esp_app_desc_t desc;
 if(!part||esp_ota_get_partition_description(part,&desc)!=ESP_OK){strcpy(message,"Running application could not be identified.");return false;}
 if(SD.totalBytes()<SD.usedBytes()+part->size+32768){strcpy(message,"Not enough free space for a verified backup.");return false;}
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
 if(!engine||!writeCurrentLog(*engine)){strcpy(message,"Current LOG snapshot failed. Backup incomplete.");return false;}
 if(!writeOperational(engine)){strcpy(message,"Operational settings write failed. Backup incomplete.");return false;}
 if(!ReadableLogs::start(ReadableLogs::Mode::BACKUP_INTERNAL)){strcpy(message,"Readable history preparation failed. Backup incomplete.");return false;}
 preparingLogs=true;began=now;strcpy(message,"Exporting complete readable history; scanning continues.");return true;
}
void tick(){
 if(!busy())return;if(uint32_t(millis()-began)>300000){cancel();strcpy(message,"Backup timed out. Incomplete slot retained.");return;}
 if(preparingLogs){ReadableLogs::tick();if(ReadableLogs::busy())return;preparingLogs=false;if(!ReadableLogs::succeeded()){job.phase=Copy::Phase::FAILED;snprintf(message,sizeof message,"Readable history failed: %.70s",ReadableLogs::status());return;}if(!copyReadableSet()){job.phase=Copy::Phase::FAILED;strcpy(message,"Readable history copy failed. Backup incomplete.");return;}pathFor("firmware.partial");file=SD.open(path,FILE_WRITE);if(!file){job.phase=Copy::Phase::FAILED;strcpy(message,"Cannot create firmware file.");return;}job.start(part->size);strcpy(message,"Logs and user state saved. Copying running app.");return;}
 Source source;Dest dest;job.tick(source,dest);
 if(job.phase==Copy::Phase::FAILED){file.close();strcpy(message,"Read/write or verification failed. No complete backup.");return;}
 if(job.phase==Copy::Phase::VERIFY)strcpy(message,"Reading back firmware to verify SHA-256.");
 if(job.phase!=Copy::Phase::DONE)return;file.close();
 char from[56];pathFor("firmware.partial");strcpy(from,path);pathFor("firmware.bin");bool ok=SD.rename(from,path);
 char hex[65];for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",job.digest[i]);
 // The app backup deliberately excludes bootloader, NVS, keys, history and
 // partition-table writes. Recovery uses the matching release kit on a PC.
 char manifest[1024];int n=snprintf(manifest,sizeof manifest,
  "DNSP_BACKUP_V2\nfirmware=DNSquachWatch v1.1.2\nbase=SquachWatch v1.25.0\nbuild=%s\napp_bytes=%lu\nsource_address=0x%lx\nsha256=%s\napp_only=true\npartition_sector_sha256=%s\npreferences=public-v2\noperational_state=operational-state.bin\nreadable_history=complete\ncurrent_log=current-log.csv\ncurrent_log_rows=%u\nUse the matching board release kit and DNSQUACHWATCH INSTALLATION.txt. Hash is integrity, not authenticity.\nPINs, Duress state and authentication secrets are never included. Device identifiers, user labels and active Watch/Hunt targets are included. Existing microSD research files are not duplicated. Historical logs are readable and are not replayed during restore.\n",
  OtaCore::buildName(),(unsigned long)part->size,(unsigned long)part->address,hex,layoutHash,(unsigned)logRows);
 ok=ok&&writeInstallationGuide();
 ok=ok&&n>0&&size_t(n)<sizeof manifest&&writeFile("COMPLETE.txt",manifest,n);
 if(ok){pathFor("COMPLETE.txt");File check=SD.open(path,FILE_READ);char b[1024];ok=check&&check.size()==size_t(n)&&check.read((uint8_t*)b,n)==n&&!memcmp(b,manifest,n);check.close();}
 if(ok){verified=true;snprintf(message,sizeof message,"Verified backup in /dnsp-backup-%u. Copy it to a computer.",selected);}else{pathFor("COMPLETE.txt");SD.remove(path);job.phase=Copy::Phase::FAILED;strcpy(message,"Manifest verification failed. Backup incomplete.");}
}
bool exportHealth(bool card,const char* report){
 healthPath[0]=0;
 if(!card||busy()||!report||strlen(report)>1536)return false;
 const char* folder="/Device Health Export";
 if(!SD.exists(folder)&&!SD.mkdir(folder))return false;
 char partial[72];snprintf(partial,sizeof partial,"%s/.device-health.partial",folder);
 if(SD.exists(partial))SD.remove(partial);
 unsigned picked=0;
 for(unsigned i=1;i<=9999;i++){
  snprintf(healthPath,sizeof healthPath,"%s/device-health-%04u.txt",folder,i);
  if(!SD.exists(healthPath)){picked=i;break;}
 }
 if(!picked){healthPath[0]=0;return false;}
 File f=SD.open(partial,FILE_WRITE);if(!f){healthPath[0]=0;return false;}size_t n=strlen(report);bool ok=f.write((const uint8_t*)report,n)==n;f.flush();f.close();
 if(!ok||!SD.rename(partial,healthPath)){SD.remove(partial);healthPath[0]=0;return false;}
 return true;
}
bool restore(bool card,DetectionEngine* engine){
 if(busy()||!card){strcpy(message,"Stop backup and mount microSD first.");return false;}
 RestoreJournal pending{};if(readJournal(pending))selected=pending.slot;
 snprintf(dir,sizeof dir,"/dnsp-backup-%u",selected);pathFor("COMPLETE.txt");if(!SD.exists(path)){strcpy(message,"No completed backup in this slot.");return false;}
 pathFor("preferences.txt");File f=SD.open(path,FILE_READ);char b[Care::SETTINGS_CAP];size_t n=f?f.size():0;
 bool ok=n>0&&n<sizeof b&&f.read((uint8_t*)b,n)==int(n);f.close();Care::Snapshot s;
 if(!ok||!Care::decode(b,n,s)){strcpy(message,"Invalid or incompatible preferences. Nothing applied.");return false;}
 pathFor("operational-state.bin");File o=SD.open(path,FILE_READ);OperationalState st{};bool op=o&&o.size()==sizeof st&&o.read((uint8_t*)&st,sizeof st)==sizeof st;o.close();uint32_t c=st.crc;st.crc=0;op=op&&Care::crc(&st,sizeof st)==c&&validOperational(st);if(!op){strcpy(message,"Invalid operational backup. Nothing applied.");return false;}
 if(!writeJournal(selected,1)){strcpy(message,"Could not start restore journal. Nothing applied.");return false;}
 ok=Care::apply(s);if(ok)ok=writeJournal(selected,2);
 if(ok){
  ok=saveFieldVerified(st.field);
  if(ok){IgnoreList::clear();for(uint8_t i=0;ok&&i<st.ignoreN;i++)ok=IgnoreList::add(st.ignore[i],(DetectionType)st.ignore[i][6]);ok=ok&&IgnoreList::count()==st.ignoreN;}
  if(ok){UserLabels::clearAll();for(uint8_t i=0;ok&&i<st.labelN;i++)ok=UserLabels::restore(st.labelMac[i],st.labels[i]);ok=ok&&UserLabels::count()==st.labelN;}
  if(ok){ScanProfile::restore((ScanProfile::Profile)st.profile,st.wifi,st.ble,st.maxMin);SketchyRule::setEnabled(st.ruleOn);const ScanProfile::Profile expected=st.profile==(uint8_t)ScanProfile::MAXIMUM?ScanProfile::BALANCED:(ScanProfile::Profile)st.profile;ok=ScanProfile::current()==expected&&ScanProfile::customWifiMs()==st.wifi&&ScanProfile::customBleShare()==st.ble&&ScanProfile::maximumMinutes()==st.maxMin&&SketchyRule::enabled()==bool(st.ruleOn);}
  if(ok)ok=saveTargets(st)&&writeJournal(selected,3);
 }
 if(ok&&engine){TargetHandoff h;h.watchKind=st.watchKind;h.huntKind=st.huntKind;memcpy(h.watchMac,st.watchMac,6);memcpy(h.huntMac,st.huntMac,6);memcpy(h.watchLabel,st.watchLabel,sizeof h.watchLabel);memcpy(h.huntLabel,st.huntLabel,sizeof h.huntLabel);applyTargets(*engine,h);}
 if(ok){clearJournal();strcpy(message,"Settings, labels, rules and targets restored. Targets will be handed across one reboot.");}else strcpy(message,"Restore was interrupted. Retry Restore; the journal keeps the same backup slot.");return ok;
}
void applyPendingTargets(DetectionEngine& engine){Preferences p;TargetHandoff h;if(!p.begin("dnsp-rstr-tgt",false))return;bool ok=p.getBytesLength("once")==sizeof h&&p.getBytes("once",&h,sizeof h)==sizeof h;uint32_t c=h.crc;h.crc=0;ok=ok&&h.magic==0x31544744u&&validKind(h.watchKind)&&validKind(h.huntKind)&&Care::crc(&h,sizeof h)==c;if(ok)applyTargets(engine,h);if(ok||p.isKey("once"))p.remove("once");p.end();}
#else
bool start(bool,uint32_t,const DetectionEngine*){strcpy(message,"Simulator: physical microSD backup requires the board.");return false;}
void tick(){}void cancel(){}bool exportHealth(bool,const char*){return false;}bool restore(bool,DetectionEngine*){strcpy(message,"Simulator: restore needs microSD on the board.");return false;}
void applyPendingTargets(DetectionEngine&){}
#endif
}
