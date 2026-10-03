#include "integer_scan.h"
#include "readable_logs.h"
#include "history_id_scan.h"
#include "location_label.h"
#include "blackbox.h"
#include "care.h"
#include "settings.h"
#include "sketchy_rule.h"
#include <cstdio>
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
#include <SD.h>
#endif
namespace ReadableLogs {
namespace {
enum Phase:uint8_t{IDLE,FIND_DET,WRITE_DET,FIND_BOOT,WRITE_BOOT,WRITE_RULES,COPY_SD_LOGS,COPY_FILES,DONE,FAILED};
bool autoOwner=false;
Phase p=IDLE;Mode mode=Mode::REFRESH;char msg[128]="Ready.";char outPath[80]="/DNSP Readable Logs/Current";
uint16_t scanAt=0,newDet=0,writeAt=0,bootAt=0,newBoot=0;uint32_t lastDet=0,lastBoot=0,nextDet=0,nextBoot=0,lastRule=0,nextRule=0,checkpointGeneration=0;uint16_t copyIx=0,sdIx=0;
uint32_t activity=0;uint8_t detLeaf=0;uint32_t ruleCeiling=0;
uint32_t hash(const void* v,size_t n){const uint8_t* q=(const uint8_t*)v;uint32_t h=2166136261u;while(n--){h^=*q++;h*=16777619u;}return h;}
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
bool dirs(){if(!SD.cardSize())return false;if(!SD.exists("/DNSP Readable Logs")&&!SD.mkdir("/DNSP Readable Logs"))return false;if(!SD.exists("/DNSP Readable Logs/Current")&&!SD.mkdir("/DNSP Readable Logs/Current"))return false;if(!SD.exists("/DNSP Readable Logs/Exports")&&!SD.mkdir("/DNSP Readable Logs/Exports"))return false;return true;}
bool append(const char* leaf,const char* text){char path[112];snprintf(path,sizeof path,"/DNSP Readable Logs/Current/%s",leaf);File f=SD.open(path,FILE_APPEND);if(!f)return false;size_t n=strlen(text);bool ok=f.write((const uint8_t*)text,n)==n;f.flush();f.close();return ok;}
bool ensure(const char* leaf,const char* header){char path[112];snprintf(path,sizeof path,"/DNSP Readable Logs/Current/%s",leaf);return SD.exists(path)||append(leaf,header);}
bool containsId(const char* leaf,const char* id){char path[112];snprintf(path,sizeof path,"/DNSP Readable Logs/Current/%s",leaf);File f=SD.open(path,FILE_READ);if(!f)return false;char window[96]{};size_t keep=0;const size_t wanted=strlen(id);while(f.available()){int n=f.read((uint8_t*)window+keep,sizeof window-1-keep);if(n<=0)break;size_t used=keep+(size_t)n;window[used]=0;if(strstr(window,id)){f.close();return true;}keep=used>wanted?wanted:used;memmove(window,window+used-keep,keep);}f.close();return false;}
bool appendUnique(const char* leaf,const char* id,const char* text){return containsId(leaf,id)||append(leaf,text);}
bool saveCheckpoint();
char resumedId[32]{},liveId[32]{};bool fastRecords=false;
const char* pendingPath="/DNSP Readable Logs/Current/.pending-record-v121";
File uniqueFile;IdScan uniqueScan;bool uniqueActive=false;BlackBox::DetRecord heldDet{};BlackBox::BootRecord heldBoot{};bool haveDet=false,haveBoot=false;SketchyRule::Incident heldRule{};bool haveRule=false;BlackBox::HistorySnapshot history;
// Commit a cursor after each complete record. A durable pending ID identifies
// the sole record that might have been partially appended across a power cut.
// Normal new records avoid scanning old history; that one recovery record (or
// the first legacy migration) still gets the complete bounded duplicate scan.
void readPending(){resumedId[0]=liveId[0]=0;fastRecords=SD.exists("/DNSP Readable Logs/Current/.cooperative-v121")&&checkpointGeneration>0;if(!SD.exists(pendingPath))return;File f=SD.open(pendingPath,FILE_READ);char text[80]{};size_t n=f?f.read((uint8_t*)text,sizeof text-1):0;f.close();unsigned long crc=0;char id[32]{};if(n&&DNSP_INTEGER_SCAN(text,"%31s %lu",id,&crc)==2&&Care::crc(id,strlen(id))==uint32_t(crc)){snprintf(resumedId,sizeof resumedId,"%s",id);}else fastRecords=false;}
bool prepareRecord(const char* id){if(!strcmp(liveId,id))return true;char text[80];int n=snprintf(text,sizeof text,"%s %lu\n",id,(unsigned long)Care::crc(id,strlen(id)));File f=SD.open(pendingPath,FILE_WRITE);bool ok=f&&f.write((const uint8_t*)text,n)==size_t(n);if(f){f.flush();f.close();}if(!ok)return false;f=SD.open(pendingPath,FILE_READ);char check[80]{};ok=f&&f.size()==size_t(n)&&f.read((uint8_t*)check,n)==n&&!memcmp(check,text,n);f.close();if(ok)snprintf(liveId,sizeof liveId,"%s",id);return ok;}
bool commitRecord(){if(!saveCheckpoint())return false;liveId[0]=0;if(SD.exists(pendingPath)&&!SD.remove(pendingPath))return false;return true;}
// Caller retries the same record/leaf until done; no full-file work in a tick.
int appendUniqueStep(const char* leaf,const char* id,const char* text){
 if(!prepareRecord(id))return -1;
 if(fastRecords&&strcmp(id,resumedId))return append(leaf,text)?1:-1;
 if(!uniqueActive){char path[112];snprintf(path,sizeof path,"/DNSP Readable Logs/Current/%s",leaf);uniqueFile=SD.open(path,FILE_READ);uniqueScan.reset();uniqueActive=true;if(!uniqueFile&&SD.exists(path)){uniqueActive=false;return -1;}}
 auto result=uniqueFile?uniqueScan.tick(uniqueFile,id):IdScan::ABSENT;
 if(result==IdScan::WAIT)return 0;
 uniqueFile.close();uniqueActive=false;
 if(result==IdScan::ERROR)return -1;
 return result==IdScan::FOUND||append(leaf,text)?1:-1;
}

bool parseCheckpoint(const char* path,uint32_t& generation,uint32_t& det,uint32_t& boot,uint32_t& rule){File f=SD.open(path,FILE_READ);if(!f)return false;char b[160]{};size_t n=f.read((uint8_t*)b,sizeof b-1);f.close();b[n]=0;unsigned long g=0,d=0,q=0,r=0,c=0;int got=DNSP_INTEGER_SCAN(b,"DNSP_CHECKPOINT_V2\ngen=%lu\ndet=%lu\nboot=%lu\nrule=%lu\ncrc=%lu\n",&g,&d,&q,&r,&c);if(got!=5)return false;char body[128];int bodyN=snprintf(body,sizeof body,"DNSP_CHECKPOINT_V2\ngen=%lu\ndet=%lu\nboot=%lu\nrule=%lu\n",g,d,q,r);if(bodyN<=0||Care::crc(body,(size_t)bodyN)!=(uint32_t)c)return false;generation=(uint32_t)g;det=(uint32_t)d;boot=(uint32_t)q;rule=(uint32_t)r;return true;}
void readCheckpoint(){const char* paths[]={"/DNSP Readable Logs/Current/.checkpoint","/DNSP Readable Logs/Current/.checkpoint.new","/DNSP Readable Logs/Current/.checkpoint.old"};for(const char* path:paths){uint32_t g=0,d=0,b=0,r=0;if(parseCheckpoint(path,g,d,b,r)&&g>=checkpointGeneration){checkpointGeneration=g;lastDet=d;lastBoot=b;lastRule=r;}}if(checkpointGeneration){if(!SD.exists("/DNSP Readable Logs/Current/.location-schema-v1.2"))lastDet=0;return;}File f=SD.open(paths[0],FILE_READ);if(f){char b[112]{};size_t n=f.read((uint8_t*)b,sizeof b-1);f.close();b[n]=0;unsigned long d=0,q=0,r=0;int got=DNSP_INTEGER_SCAN(b,"det=%lu\nboot=%lu\nrule=%lu",&d,&q,&r);if(got>=2){lastDet=(uint32_t)d;lastBoot=(uint32_t)q;if(got>=3)lastRule=(uint32_t)r;}}}
bool saveCheckpoint(){char body[128],all[160];uint32_t generation=checkpointGeneration+1;int bodyN=snprintf(body,sizeof body,"DNSP_CHECKPOINT_V2\ngen=%lu\ndet=%lu\nboot=%lu\nrule=%lu\n",(unsigned long)generation,(unsigned long)nextDet,(unsigned long)nextBoot,(unsigned long)nextRule);int n=snprintf(all,sizeof all,"%scrc=%lu\n",body,(unsigned long)Care::crc(body,(size_t)bodyN));if(bodyN<=0||n<=0||n>=(int)sizeof all)return false;const char* fresh="/DNSP Readable Logs/Current/.checkpoint.new";const char* current="/DNSP Readable Logs/Current/.checkpoint";const char* old="/DNSP Readable Logs/Current/.checkpoint.old";SD.remove(fresh);File f=SD.open(fresh,FILE_WRITE);bool ok=f&&f.write((const uint8_t*)all,n)==(size_t)n;if(f){f.flush();f.close();}if(!ok)return false;uint32_t vg=0,vd=0,vb=0,vr=0;if(!parseCheckpoint(fresh,vg,vd,vb,vr)||vg!=generation)return false;SD.remove(old);if(SD.exists(current)&&!SD.rename(current,old))return false;if(!SD.rename(fresh,current)){if(SD.exists(old))SD.rename(old,current);return false;}SD.remove(old);checkpointGeneration=generation;return ensure(".location-schema-v1.2","1\n");}
void mac(char* b,size_t n,const uint8_t* m){snprintf(b,n,"%02X:%02X:%02X:%02X:%02X:%02X",m[0],m[1],m[2],m[3],m[4],m[5]);}
int writeDet(const BlackBox::DetRecord&r){char m[24],id[32],line[448];mac(m,sizeof m,r.mac);snprintf(id,sizeof id,"D-%u-%lu-%08lx",(unsigned)r.boot,(unsigned long)r.upSec,(unsigned long)hash(&r,sizeof r));snprintf(line,sizeof line,"Record %u:%lu | %s%s | %s | RSSI %d | channel %u | hits %u | %s | %s | Location %s | ID %s\n",(unsigned)r.boot,(unsigned long)r.upSec,detectionTypeName((DetectionType)r.type),(r.flags&BlackBox::DET_AGAIN)?" reappeared":"",m,(int)r.rssi,(unsigned)r.channel,(unsigned)r.hits,r.vendor,r.name,LocationLabel::text(BlackBox::locationKey(r)),id);if(detLeaf<2){int q=appendUniqueStep(detLeaf?"ALL-ALERTS.txt":"SCAN-HISTORY.txt",id,line);if(q<0)return -1;if(q>0)++detLeaf;return 0;}snprintf(line,sizeof line,"%u,%lu,%s,%s,%d,%u,%u,%u,%s,%s,%s,%s\n",(unsigned)r.boot,(unsigned long)r.upSec,detectionTypeName((DetectionType)r.type),m,(int)r.rssi,(unsigned)r.channel,(unsigned)r.hits,(unsigned)r.flags,r.vendor,r.name,LocationLabel::text(BlackBox::locationKey(r)),id);int q=appendUniqueStep("ALERT-HISTORY-v1.2.csv",id,line);if(q>0)detLeaf=0;return q;}
int writeBoot(const BlackBox::BootRecord&r){char id[32],line[392];snprintf(id,sizeof id,"B-%u-%08lx",(unsigned)r.boot,(unsigned long)hash(&r,sizeof r));snprintf(line,sizeof line,"Boot %u | %s | firmware %s | prior uptime %lus | heap %lu/%lu | screen %u | task %s | pc %08lx | cause %lu | Location %s | ID %s\n",(unsigned)r.boot,BlackBox::reasonName(r.reason),r.version,(unsigned long)r.upSec,(unsigned long)r.heapFree,(unsigned long)r.heapBlock,(unsigned)r.screen,r.task,(unsigned long)r.pc,(unsigned long)r.cause,"no-label-set",id);return appendUniqueStep("SYSTEM-HISTORY.txt",id,line);}
bool copyFile(const char* leaf){char from[112],to[128];snprintf(from,sizeof from,"/DNSP Readable Logs/Current/%s",leaf);snprintf(to,sizeof to,"%s/%s",outPath,leaf);File a=SD.open(from,FILE_READ);if(!a)return true;File b=SD.open(to,FILE_WRITE);if(!b){a.close();return false;}uint8_t buf[512];bool ok=true;while(a.available()){int n=a.read(buf,sizeof buf);if(n<=0||b.write(buf,n)!=(size_t)n){ok=false;break;}}b.flush();a.close();b.close();return ok;}
int copyCurrentAt(uint16_t wanted){File root=SD.open("/DNSP Readable Logs/Current");if(!root)return -1;uint16_t seen=0;char leaf[96]{};for(File f=root.openNextFile();f;f=root.openNextFile()){const char*n=f.name();const char*b=strrchr(n,'/');b=b?b+1:n;if(!f.isDirectory()&&b[0]!='.'&&seen++==wanted){snprintf(leaf,sizeof leaf,"%s",b);f.close();break;}f.close();}root.close();if(!leaf[0])return 0;return copyFile(leaf)?1:-1;}
char sdSourceName[64]{},sdStatePath[128]{};uint32_t sdOffset=0,sdDestSize=0,sdCeiling=0;bool sdOpenError=false;
bool logName(const char* n){const char* b=strrchr(n,'/');b=b?b+1:n;return !strncmp(b,"squachwatch-",12)||!strncmp(b,"dnsp-health",11)||!strncmp(b,"dnsp-research",13)||!strncmp(b,"dnsp-rid-capture",16)||!strcmp(b,"dnsp-field-report.txt")||!strcmp(b,"dnsp-fpv-pit.csv");}
bool saveSdState(){char body[112],all[144],fresh[132],old[132];int bn=snprintf(body,sizeof body,"DNSP_SD_COPY_V1\noffset=%lu\ndest=%lu\n",(unsigned long)sdOffset,(unsigned long)sdDestSize);int n=snprintf(all,sizeof all,"%scrc=%lu\n",body,(unsigned long)Care::crc(body,(size_t)bn));snprintf(fresh,sizeof fresh,"%s.new",sdStatePath);snprintf(old,sizeof old,"%s.old",sdStatePath);SD.remove(fresh);File f=SD.open(fresh,FILE_WRITE);bool ok=f&&f.write((const uint8_t*)all,n)==(size_t)n;if(f){f.flush();f.close();}if(!ok)return false;SD.remove(old);if(SD.exists(sdStatePath)&&!SD.rename(sdStatePath,old))return false;if(!SD.rename(fresh,sdStatePath)){if(SD.exists(old))SD.rename(old,sdStatePath);return false;}SD.remove(old);return true;}
bool readSdState(uint32_t& offset,uint32_t& destSize){const char* suffixes[]={"",".new",".old"};bool found=false;for(const char*s:suffixes){char path[136];snprintf(path,sizeof path,"%s%s",sdStatePath,s);File f=SD.open(path,FILE_READ);if(!f)continue;char b[144]{};size_t n=f.read((uint8_t*)b,sizeof b-1);f.close();b[n]=0;unsigned long o=0,d=0,c=0;if(DNSP_INTEGER_SCAN(b,"DNSP_SD_COPY_V1\noffset=%lu\ndest=%lu\ncrc=%lu\n",&o,&d,&c)!=3)continue;char body[112];int bn=snprintf(body,sizeof body,"DNSP_SD_COPY_V1\noffset=%lu\ndest=%lu\n",o,d);if(Care::crc(body,(size_t)bn)!=(uint32_t)c)continue;if(!found||o>offset){offset=(uint32_t)o;destSize=(uint32_t)d;found=true;}}return found;}
void sdDestination(char* out,size_t cap){const char* base=strrchr(sdSourceName,'/');snprintf(out,cap,"/DNSP Readable Logs/Current/SD-%s.txt",base?base+1:sdSourceName);for(char* q=strrchr(out,'/')+1;*q;q++)if(*q==' ')*q='_';}
bool openSdLog(uint16_t wanted){
 sdOpenError=false;sdSourceName[0]=0;File root=SD.open("/");if(!root){sdOpenError=true;return false;}
 uint16_t seen=0;for(File f=root.openNextFile();f;f=root.openNextFile()){
  const char* n=f.name();if(!f.isDirectory()&&logName(n)&&seen++==wanted){const char* base=strrchr(n,'/');base=base?base+1:n;
   if(strlen(base)+2>sizeof sdSourceName){f.close();root.close();sdOpenError=true;return false;}
   snprintf(sdSourceName,sizeof sdSourceName,"/%s",base);f.close();break;}f.close();
 }root.close();if(!sdSourceName[0])return false;sdOpenError=true;
 snprintf(sdStatePath,sizeof sdStatePath,"/DNSP Readable Logs/Current/.sd-%08lx.state",(unsigned long)hash(sdSourceName,strlen(sdSourceName)));
 sdOffset=sdDestSize=0;bool haveState=readSdState(sdOffset,sdDestSize);
 File source=SD.open(sdSourceName,FILE_READ);if(!source)return false;sdCeiling=source.size();source.close();
 char dest[128];sdDestination(dest,sizeof dest);File out=SD.open(dest,FILE_APPEND);if(!out)return false;uint32_t actual=out.size();out.close();
 if(!haveState){sdDestSize=actual;sdOffset=actual<=sdCeiling?actual:0;}
 if(actual<sdDestSize||sdOffset>sdCeiling){return false;}
 if(actual>sdDestSize){
  // A power cut may occur after append but before journal commit. Verify
  // that sole pending chunk; never silently discard mismatching bytes.
  uint32_t delta=actual-sdDestSize;bool same=delta<=512 && delta<=sdCeiling-sdOffset;
  File a=SD.open(sdSourceName,FILE_READ),b=SD.open(dest,FILE_READ);same=same&&a&&b&&a.seek(sdOffset)&&b.seek(sdDestSize);
  uint8_t x[64],y[64];uint32_t left=delta;while(same&&left){size_t n=left>sizeof x?sizeof x:left;same=a.read(x,n)==(int)n&&b.read(y,n)==(int)n&&!memcmp(x,y,n);left-=n;}a.close();b.close();
  if(same)sdOffset+=delta;
  else {File c=SD.open(dest,FILE_APPEND);const char* marker="\n[DNSP: interrupted bytes above retained; source chunk retried]\n";bool ok=c&&c.write((const uint8_t*)marker,strlen(marker))==strlen(marker);c.flush();actual=c.size();c.close();if(!ok)return false;}
  sdDestSize=actual;
 }
 if(!saveSdState())return false;
 char id[24],line[176];snprintf(id,sizeof id,"S-%08lx",(unsigned long)hash(sdSourceName,strlen(sdSourceName)));snprintf(line,sizeof line,"%s -> SD-%s.txt | ID %s\n",sdSourceName,sdSourceName+1,id);
 if(!appendUnique("MICROSD-FILE-INDEX.txt",id,line))return false;
 sdOpenError=false;return true;
}
// Never hold files across ticks: ordinary SD logging and journal writes need
// a free handle. Capture a source ceiling so an active growing log terminates.
int copySdStep(){
 if(sdOffset>=sdCeiling){sdSourceName[0]=0;return 1;}
 uint8_t block[512];size_t want=sdCeiling-sdOffset;if(want>sizeof block)want=sizeof block;
 File source=SD.open(sdSourceName,FILE_READ);bool ok=source&&source.seek(sdOffset)&&source.read(block,want)==(int)want;source.close();if(!ok)return -1;
 char dest[128];sdDestination(dest,sizeof dest);File out=SD.open(dest,FILE_APPEND);ok=out&&out.size()==sdDestSize&&out.write(block,want)==want;out.flush();uint32_t actual=out.size();out.close();if(!ok)return -1;
 sdOffset+=want;sdDestSize=actual;return saveSdState()?0:-1;
}

#endif
}
bool start(Mode m){if(busy())return false;autoOwner=false;mode=m;scanAt=newDet=writeAt=bootAt=newBoot=0;lastDet=lastBoot=nextDet=nextBoot=lastRule=nextRule=0;copyIx=sdIx=0;checkpointGeneration=0;activity=0;detLeaf=0;ruleCeiling=0;
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
 SketchyRule::Incident newestRule{};if(SketchyRule::recent(0,newestRule))ruleCeiling=newestRule.id;
 sdSourceName[0]=0;BlackBox::captureHistory(history);haveDet=haveBoot=haveRule=false;uniqueFile.close();uniqueActive=false;
 if(!dirs()){p=FAILED;strcpy(msg,"microSD unavailable or folders could not be created.");return false;}readCheckpoint();readPending();nextDet=lastDet;nextBoot=lastBoot;nextRule=lastRule;
 if(!ensure("ALERT-HISTORY-v1.2.csv","boot,uptime_s,type,mac,rssi,channel,hits,flags,vendor,name,location,record_id\n")||!ensure("ALL-ALERTS.txt","DNSquachWatch stored alert history\n")||!ensure("SCAN-HISTORY.txt","DNSquachWatch stored scan history\n")||!ensure("SYSTEM-HISTORY.txt","DNSquachWatch boot, crash and system history\n")){p=FAILED;strcpy(msg,"Readable files could not be created.");return false;}
 p=FIND_DET;strcpy(msg,"Finding new stored scan events...");strcpy(outPath,"/DNSP Readable Logs/Current");return true;
#else
 p=FAILED;strcpy(msg,"Physical microSD required.");return false;
#endif
}
void tick(){
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
 if(!busy())return;++activity;if(!BlackBox::historyIntact(history)){uniqueFile.close();uniqueActive=false;p=FAILED;strcpy(msg,"Stored history wrapped during export. Retry to capture the new head.");return;}
 if(p==FIND_DET){BlackBox::DetRecord r{};if(!BlackBox::readDetectionsSnapshot(history,scanAt,1,&r)){newDet=scanAt;writeAt=newDet;p=WRITE_DET;return;}uint32_t h=hash(&r,sizeof r);if(scanAt==0)nextDet=h;if(lastDet&&h==lastDet){newDet=scanAt;writeAt=newDet;p=WRITE_DET;return;}if(++scanAt>=BlackBox::detectionsKept()){newDet=scanAt;writeAt=newDet;p=WRITE_DET;}return;}
 if(p==WRITE_DET){if(!writeAt){p=FIND_BOOT;strcpy(msg,"Finding new device and system records...");return;}if(!haveDet)haveDet=BlackBox::readDetectionsSnapshot(history,writeAt-1,1,&heldDet);int q=haveDet?writeDet(heldDet):-1;if(q<0){p=FAILED;strcpy(msg,"Could not write readable scan history.");}else if(q>0){nextDet=hash(&heldDet,sizeof heldDet);nextBoot=lastBoot;if(!commitRecord()){p=FAILED;strcpy(msg,"Scan cursor commit failed; retry is safe.");return;}--writeAt;haveDet=false;}return;}
 if(p==FIND_BOOT){BlackBox::BootRecord r{};if(!BlackBox::readBootsSnapshot(history,bootAt,1,&r)){newBoot=bootAt;writeAt=newBoot;p=WRITE_BOOT;return;}uint32_t h=hash(&r,sizeof r);if(bootAt==0)nextBoot=h;if(lastBoot&&h==lastBoot){newBoot=bootAt;writeAt=newBoot;p=WRITE_BOOT;return;}if(++bootAt>=BlackBox::bootsKept()){newBoot=bootAt;writeAt=newBoot;p=WRITE_BOOT;}return;}
 if(p==WRITE_BOOT){if(!writeAt){p=WRITE_RULES;strcpy(msg,"Writing rule alerts and export summary...");return;}if(!haveBoot)haveBoot=BlackBox::readBootsSnapshot(history,writeAt-1,1,&heldBoot);int q=haveBoot?writeBoot(heldBoot):-1;if(q<0){p=FAILED;strcpy(msg,"Could not write readable system history.");}else if(q>0){nextBoot=hash(&heldBoot,sizeof heldBoot);if(!commitRecord()){p=FAILED;strcpy(msg,"System cursor commit failed; retry is safe.");return;}--writeAt;haveBoot=false;}return;}
 if(p==WRITE_RULES){if(!ensure("SKETCHY-ENVIRONMENT.txt","DNSquachWatch Sketchy Environment rule history\n")){p=FAILED;strcpy(msg,"Could not create rule history.");return;}if(!haveRule){for(uint8_t i=0;i<SketchyRule::count();i++){SketchyRule::Incident candidate{};if(SketchyRule::recent(i,candidate)&&candidate.id>nextRule&&candidate.id<=ruleCeiling&&(!haveRule||candidate.id<heldRule.id)){heldRule=candidate;haveRule=true;}}}if(haveRule){const auto& in=heldRule;if(in.id>nextRule)nextRule=in.id;char a[24],d[24],id[28],line[384];mac(a,sizeof a,in.alpr.mac);mac(d,sizeof d,in.deauth.mac);snprintf(id,sizeof id,"R-%010lu",(unsigned long)in.id);snprintf(line,sizeof line,"Incident %lu | %s %s + DEAUTH %s | gap %lus | microSD %s | ALPR location %s | DEAUTH location %s | ID %s\n",(unsigned long)in.id,detectionTypeName(in.alpr.type),a,d,(unsigned long)in.gapSeconds,in.sdExported?"saved":"pending",LocationLabel::text(in.alpr.locationKey),LocationLabel::text(in.deauth.locationKey),id);int q=appendUniqueStep("SKETCHY-ENVIRONMENT.txt",id,line);if(q<0){p=FAILED;strcpy(msg,"Could not write rule history.");return;}if(q>0){if(!commitRecord()){p=FAILED;strcpy(msg,"Rule cursor commit failed; retry is safe.");return;}haveRule=false;}return;}if(nextRule<lastRule)nextRule=lastRule;
  char sum[420];snprintf(sum,sizeof sum,"DNSquachWatch readable log collection\nNew scan records this refresh: %u\nNew system records: %u\nStored scan records currently on board: %u\nStored system records currently on board: %u\nRule incidents currently retained: %u\nRecords are ordered by boot number and uptime because this board has no reliable clock. Original data remains unchanged.\n",(unsigned)newDet,(unsigned)newBoot,(unsigned)BlackBox::detectionsKept(),(unsigned)BlackBox::bootsKept(),(unsigned)SketchyRule::count());SD.remove("/DNSP Readable Logs/Current/EXPORT-SUMMARY.txt");if(!append("EXPORT-SUMMARY.txt",sum)||!saveCheckpoint()||!ensure(".cooperative-v121","1\n")){p=FAILED;strcpy(msg,"Readable files were written, but the refresh checkpoint failed.");return;}
  if(mode==Mode::BACKUP_INTERNAL){p=DONE;strcpy(msg,"Internal readable history prepared for backup.");return;}p=COPY_SD_LOGS;strcpy(msg,"Updating readable copies of existing microSD logs...");return;
 }
 if(p==COPY_SD_LOGS){if(sdSourceName[0]){int q=copySdStep();if(q<0){sdSourceName[0]=0;p=FAILED;strcpy(msg,"microSD copy failed; retry retains completed bytes.");return;}if(q>0)++sdIx;return;}if(openSdLog(sdIx))return;if(sdOpenError){sdSourceName[0]=0;p=FAILED;strcpy(msg,"A microSD source or copy journal could not be opened.");return;}
  if(mode==Mode::REFRESH){p=DONE;strcpy(msg,"Readable files refreshed. Duress PIN options are under Security.");return;}
  unsigned n=0;for(unsigned i=1;i<=9999;i++){snprintf(outPath,sizeof outPath,"/DNSP Readable Logs/Exports/Export-%04u",i);if(!SD.exists(outPath)){n=i;break;}}if(!n||!SD.mkdir(outPath)){p=FAILED;strcpy(msg,"Could not create an organized export folder.");return;}p=COPY_FILES;copyIx=0;strcpy(msg,"Copying organized readable snapshot...");return;
 }
 if(p==COPY_FILES){int q=copyCurrentAt(copyIx);if(q>0){++copyIx;return;}if(q<0){p=FAILED;strcpy(msg,"Organized snapshot copy failed.");return;}p=DONE;strcpy(msg,"Organized snapshot complete. Duress PIN options are under Security.");return;}
#endif
}
void cancel(){if(busy()){
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
 sdSourceName[0]=0;uniqueFile.close();uniqueActive=false;
#endif
 p=FAILED;strcpy(msg,"Readable-log operation cancelled; completed files were kept.");}}
bool automatic(){return autoOwner;}
void automaticTick(uint32_t now,bool available){
 static uint32_t last=0;
 if(autoOwner&&busy()){if(!available||!Settings::autoHistory()){cancel();autoOwner=false;return;}tick();return;}
 if(!busy())autoOwner=false;
 if(!available||!Settings::autoHistory()||busy()||now-last<60000u)return;
 last=now;autoOwner=start(Mode::REFRESH);
}
bool busy(){return p>=FIND_DET&&p<=COPY_FILES;}unsigned percent(){if(p==DONE)return 100;if(!busy())return 0;if(p<=WRITE_DET)return 10+(BlackBox::detectionsKept()?40u*(p==FIND_DET?scanAt:newDet-writeAt)/BlackBox::detectionsKept():40);if(p<=WRITE_BOOT)return 55+(BlackBox::bootsKept()?20u*(p==FIND_BOOT?bootAt:newBoot-writeAt)/BlackBox::bootsKept():20);if(p==WRITE_RULES)return 78;if(p==COPY_SD_LOGS)return 82;if(p==COPY_FILES){unsigned q=84u+2u*copyIx;return q>99?99:q;}return 0;}
uint32_t progressToken(){return activity;}
bool succeeded(){return p==DONE;}
const char* phase(){return p<=WRITE_DET?"SCAN LOGS":p<=WRITE_BOOT?"SYSTEM LOGS":p==WRITE_RULES?"RULE LOGS":p==COPY_SD_LOGS?"MICROSD LOGS":p==COPY_FILES?"ORGANIZING":"READY";}const char* status(){return msg;}const char* outputPath(){return outPath;}
}
