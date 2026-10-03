#include "simulation.h"
#include "csv_text.h"
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
#include <Arduino.h>
#endif
namespace ReadableLogs {
namespace {
enum Phase:uint8_t{IDLE,FIND_DET,WRITE_DET,FIND_BOOT,WRITE_BOOT,WRITE_RULES,COMMIT_RECORD,FINALIZE,COPY_SD_LOGS,COPY_FILES,DONE,FAILED};
bool autoOwner=false;Phase commitFrom=IDLE;
Phase p=IDLE;Mode mode=Mode::REFRESH;char msg[128]="Ready.";char outPath[80]="/DNSP Readable Logs/Current";
uint16_t scanAt=0,newDet=0,writeAt=0,bootAt=0,newBoot=0;uint32_t lastDet=0,lastBoot=0,nextDet=0,nextBoot=0,lastRule=0,nextRule=0,checkpointGeneration=0;uint16_t copyIx=0,sdIx=0;
const char* lastOperation="none";uint32_t lastOperationUs=0;const char* lastStep="idle";
void resetTiming(const char* step){lastOperation="none";lastOperationUs=0;lastStep=step;}
uint32_t activity=0;uint8_t detLeaf=0;uint32_t ruleCeiling=0;
uint32_t hash(const void* v,size_t n){const uint8_t* q=(const uint8_t*)v;uint32_t h=2166136261u;while(n--){h^=*q++;h*=16777619u;}return h;}
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)

// Record only the slowest operation in the current step. Unsigned elapsed time
// is rollover-safe; these wrappers allocate nothing and print nothing.
struct CardTiming{const char* name;uint32_t start;explicit CardTiming(const char* n):name(n),start(micros()){}~CardTiming(){uint32_t us=micros()-start;if(us>lastOperationUs){lastOperationUs=us;lastOperation=name;}}};
File cardOpen(const char* path,decltype(FILE_READ+0) mode=FILE_READ){CardTiming t("open");return SD.open(path,mode);}
bool cardExists(const char* path){CardTiming t("exists");return SD.exists(path);}
bool cardRemove(const char* path){CardTiming t("remove");return SD.remove(path);}
bool cardRename(const char* a,const char* b){CardTiming t("rename");return SD.rename(a,b);}
bool cardMkdir(const char* path){CardTiming t("mkdir");return SD.mkdir(path);}
int cardRead(File& f,uint8_t* b,size_t n){CardTiming t("read");return f.read(b,n);}
size_t cardWrite(File& f,const uint8_t* b,size_t n){CardTiming t("write");return f.write(b,n);}
bool cardSeek(File& f,uint32_t offset){CardTiming t("seek");return f.seek(offset);}
void cardFlush(File& f){CardTiming t("flush");f.flush();}
void cardClose(File& f){CardTiming t("close");f.close();}
size_t cardSize(File& f){CardTiming t("size");return f.size();}
File cardNext(File& f){CardTiming t("directory next");return f.openNextFile();}
bool dirs(){if(!SD.cardSize())return false;if(!cardExists("/DNSP Readable Logs")&&!cardMkdir("/DNSP Readable Logs"))return false;if(!cardExists("/DNSP Readable Logs/Current")&&!cardMkdir("/DNSP Readable Logs/Current"))return false;if(!cardExists("/DNSP Readable Logs/Exports")&&!cardMkdir("/DNSP Readable Logs/Exports"))return false;return true;}
bool append(const char* leaf,const char* text){char path[112];snprintf(path,sizeof path,"/DNSP Readable Logs/Current/%s",leaf);File f=cardOpen(path,FILE_APPEND);if(!f)return false;size_t n=strlen(text);bool ok=cardWrite(f,(const uint8_t*)text,n)==n;cardFlush(f);cardClose(f);return ok;}
bool ensure(const char* leaf,const char* header){char path[112];snprintf(path,sizeof path,"/DNSP Readable Logs/Current/%s",leaf);return cardExists(path)||append(leaf,header);}
bool containsId(const char* leaf,const char* id){char path[112];snprintf(path,sizeof path,"/DNSP Readable Logs/Current/%s",leaf);File f=cardOpen(path,FILE_READ);if(!f)return false;char window[96]{};size_t keep=0;const size_t wanted=strlen(id);while(f.available()){int n=cardRead(f,(uint8_t*)window+keep,sizeof window-1-keep);if(n<=0)break;size_t used=keep+(size_t)n;window[used]=0;if(strstr(window,id)){cardClose(f);return true;}keep=used>wanted?wanted:used;memmove(window,window+used-keep,keep);}cardClose(f);return false;}
bool appendUnique(const char* leaf,const char* id,const char* text){return containsId(leaf,id)||append(leaf,text);}
uint8_t finishStep=0,checkpointStep=0;bool pendingVerify=false,checkpointCurrent=true,locationSchemaReady=false;
char resumedId[32]{},liveId[32]{};bool fastRecords=false;
const char* pendingPath="/DNSP Readable Logs/Current/.pending-record-v121";
File uniqueFile;IdScan uniqueScan;bool uniqueActive=false;BlackBox::DetRecord heldDet{};BlackBox::BootRecord heldBoot{};bool haveDet=false,haveBoot=false;SketchyRule::Incident heldRule{};bool haveRule=false;BlackBox::HistorySnapshot history;BlackBox::HistoryCursor detectionCursor,bootCursor,lastNewDetection,lastNewBoot;
// Commit a cursor after each complete record. A durable pending ID identifies
// the sole record that might have been partially appended across a power cut.
// Normal new records avoid scanning old history; that one recovery record (or
// the first legacy migration) still gets the complete bounded duplicate scan.
void readPending(){resumedId[0]=liveId[0]=0;fastRecords=cardExists("/DNSP Readable Logs/Current/.cooperative-v121")&&checkpointGeneration>0;if(!cardExists(pendingPath))return;File f=cardOpen(pendingPath,FILE_READ);char text[80]{};size_t n=f?cardRead(f,(uint8_t*)text,sizeof text-1):0;cardClose(f);unsigned long crc=0;char id[32]{};if(n&&DNSP_INTEGER_SCAN(text,"%31s %lu",id,&crc)==2&&Care::crc(id,strlen(id))==uint32_t(crc)){snprintf(resumedId,sizeof resumedId,"%s",id);}else fastRecords=false;}
// Preparing, appending and committing are separate passes. SD metadata
// operations can stall for tens of milliseconds even for a tiny record.
int prepareRecord(const char* id){
 if(!strcmp(liveId,id))return 1;
 char text[80];int n=snprintf(text,sizeof text,"%s %lu\n",id,(unsigned long)Care::crc(id,strlen(id)));
 File f=cardOpen(pendingPath,pendingVerify?FILE_READ:FILE_WRITE);bool ok=false;
 if(pendingVerify){char check[80]{};ok=f&&cardSize(f)==size_t(n)&&cardRead(f,(uint8_t*)check,n)==n&&!memcmp(check,text,n);}
 else {ok=f&&cardWrite(f,(const uint8_t*)text,n)==size_t(n);if(f)cardFlush(f);}
 cardClose(f);if(!ok)return -1;
 if(!pendingVerify){pendingVerify=true;return 0;}
 pendingVerify=false;snprintf(liveId,sizeof liveId,"%s",id);return 0;
}
// Caller retries the same record/leaf until done; no full-file work in a tick.
int appendUniqueStep(const char* leaf,const char* id,const char* text){
 int prepared=prepareRecord(id);if(prepared<=0)return prepared;
 if(fastRecords&&strcmp(id,resumedId))return append(leaf,text)?1:-1;
 if(!uniqueActive){char path[112];snprintf(path,sizeof path,"/DNSP Readable Logs/Current/%s",leaf);uniqueFile=cardOpen(path,FILE_READ);uniqueScan.reset();uniqueActive=true;if(!uniqueFile&&cardExists(path)){uniqueActive=false;return -1;}}
 auto result=[&](){CardTiming t("duplicate scan");return uniqueFile?uniqueScan.tick(uniqueFile,id):IdScan::ABSENT;}();
 if(result==IdScan::WAIT)return 0;
 cardClose(uniqueFile);uniqueActive=false;
 if(result==IdScan::ERROR)return -1;
 return result==IdScan::FOUND||append(leaf,text)?1:-1;
}

bool parseCheckpoint(const char* path,uint32_t& generation,uint32_t& det,uint32_t& boot,uint32_t& rule){if(!cardExists(path))return false;File f=cardOpen(path,FILE_READ);if(!f)return false;char b[160]{};size_t n=cardRead(f,(uint8_t*)b,sizeof b-1);cardClose(f);b[n]=0;unsigned long g=0,d=0,q=0,r=0,c=0;int got=DNSP_INTEGER_SCAN(b,"DNSP_CHECKPOINT_V2\ngen=%lu\ndet=%lu\nboot=%lu\nrule=%lu\ncrc=%lu\n",&g,&d,&q,&r,&c);if(got!=5)return false;char body[128];int bodyN=snprintf(body,sizeof body,"DNSP_CHECKPOINT_V2\ngen=%lu\ndet=%lu\nboot=%lu\nrule=%lu\n",g,d,q,r);if(bodyN<=0||Care::crc(body,(size_t)bodyN)!=(uint32_t)c)return false;generation=(uint32_t)g;det=(uint32_t)d;boot=(uint32_t)q;rule=(uint32_t)r;return true;}
void readCheckpoint(){const char* paths[]={"/DNSP Readable Logs/Current/.checkpoint","/DNSP Readable Logs/Current/.checkpoint.new","/DNSP Readable Logs/Current/.checkpoint.old"};for(const char* path:paths){uint32_t g=0,d=0,b=0,r=0;if(parseCheckpoint(path,g,d,b,r)&&g>=checkpointGeneration){checkpointGeneration=g;lastDet=d;lastBoot=b;lastRule=r;checkpointCurrent=path==paths[0];}}if(checkpointGeneration){if(!cardExists("/DNSP Readable Logs/Current/ALERT-HISTORY-v1.5.3.csv"))lastDet=lastRule=0;return;}File f=cardOpen(paths[0],FILE_READ);if(f){char b[112]{};size_t n=cardRead(f,(uint8_t*)b,sizeof b-1);cardClose(f);b[n]=0;unsigned long d=0,q=0,r=0;int got=DNSP_INTEGER_SCAN(b,"det=%lu\nboot=%lu\nrule=%lu",&d,&q,&r);if(got>=2){lastDet=(uint32_t)d;lastBoot=(uint32_t)q;if(got>=3)lastRule=(uint32_t)r;}}}
// Alternate two files without rename/delete churn. Overwrite only the older
// slot; the last validated cursor survives interruption. Existing .old files
// remain readable as legacy recovery candidates. Generation and CRC format
// are unchanged, so recovery still chooses the newest complete checkpoint.
int checkpointTick(){
 const char* fresh="/DNSP Readable Logs/Current/.checkpoint.new";
 const char* current="/DNSP Readable Logs/Current/.checkpoint";
 const char* target=checkpointCurrent?fresh:current;
 switch(checkpointStep){
 case 0: {
  char body[128],all[160];uint32_t generation=checkpointGeneration+1;
  int bn=snprintf(body,sizeof body,"DNSP_CHECKPOINT_V2\ngen=%lu\ndet=%lu\nboot=%lu\nrule=%lu\n",(unsigned long)generation,(unsigned long)nextDet,(unsigned long)nextBoot,(unsigned long)nextRule);
  int n=snprintf(all,sizeof all,"%scrc=%lu\n",body,(unsigned long)Care::crc(body,(size_t)bn));
  if(bn<=0||n<=0||n>=(int)sizeof all)return -1;
  File f=cardOpen(target,FILE_WRITE);bool ok=f&&cardWrite(f,(const uint8_t*)all,n)==size_t(n);if(f){cardFlush(f);cardClose(f);}if(!ok)return -1;break;
 }
 case 1: {uint32_t g=0,d=0,b=0,r=0;if(!parseCheckpoint(target,g,d,b,r)||g!=checkpointGeneration+1)return -1;break;}
 case 2: if(!locationSchemaReady){if(!ensure(".location-schema-v1.2","1\n"))return -1;locationSchemaReady=true;}break;
 default: ++checkpointGeneration;checkpointCurrent=!checkpointCurrent;checkpointStep=0;return 1;
 }
 ++checkpointStep;return 0;
}
void beginCommit(Phase from){commitFrom=from;checkpointStep=0;p=COMMIT_RECORD;}
void mac(char* b,size_t n,const uint8_t* m){snprintf(b,n,"%02X:%02X:%02X:%02X:%02X:%02X",m[0],m[1],m[2],m[3],m[4],m[5]);}
int writeDet(const BlackBox::DetRecord&r){char m[24],id[32],line[448];mac(m,sizeof m,r.mac);snprintf(id,sizeof id,"D-%u-%lu-%08lx%s",(unsigned)r.boot,(unsigned long)r.upSec,(unsigned long)hash(&r,sizeof r),BlackBox::simulated(r)?"-S1":"");snprintf(line,sizeof line,"Record %u:%lu | %s%s | %s | RSSI %d | channel %u | hits %u | %s | %s | Location %s | %s | Address %s | ID %s\n",(unsigned)r.boot,(unsigned long)r.upSec,detectionTypeName((DetectionType)r.type),(r.flags&BlackBox::DET_AGAIN)?" reappeared":"",m,(int)r.rssi,(unsigned)r.channel,(unsigned)r.hits,r.vendor,r.name,LocationLabel::text(BlackBox::locationKey(r)),BlackBox::simulated(r)?"SIMULATED":"",Simulation::roleName(BlackBox::addressRole(r)),id);if(detLeaf<2){int q=appendUniqueStep(detLeaf?"ALL-ALERTS.txt":"SCAN-HISTORY.txt",id,line);if(q<0)return -1;if(q>0)++detLeaf;return 0;}char vendor[sizeof r.vendor],name[sizeof r.name],location[25];safeCsvText(vendor,sizeof vendor,r.vendor,sizeof r.vendor);safeCsvText(name,sizeof name,r.name,sizeof r.name);safeCsvText(location,sizeof location,LocationLabel::text(BlackBox::locationKey(r)),24);snprintf(line,sizeof line,"%u,%lu,%s,%s,%d,%u,%u,%u,%s,%s,%s,%u,%s,%s\n",(unsigned)r.boot,(unsigned long)r.upSec,detectionTypeName((DetectionType)r.type),m,(int)r.rssi,(unsigned)r.channel,(unsigned)r.hits,(unsigned)r.flags,vendor,name,location,BlackBox::simulated(r),Simulation::roleName(BlackBox::addressRole(r)),id);int q=appendUniqueStep("ALERT-HISTORY-v1.5.3.csv",id,line);if(q>0)detLeaf=0;return q;}
int writeBoot(const BlackBox::BootRecord&r){char id[32],line[392];snprintf(id,sizeof id,"B-%u-%08lx",(unsigned)r.boot,(unsigned long)hash(&r,sizeof r));snprintf(line,sizeof line,"Boot %u | %s | firmware %s | prior uptime %lus | heap %lu/%lu | screen %u | task %s | pc %08lx | cause %lu | Location %s | ID %s\n",(unsigned)r.boot,BlackBox::reasonName(r.reason),r.version,(unsigned long)r.upSec,(unsigned long)r.heapFree,(unsigned long)r.heapBlock,(unsigned)r.screen,r.task,(unsigned long)r.pc,(unsigned long)r.cause,"no-label-set",id);return appendUniqueStep("SYSTEM-HISTORY.txt",id,line);}
bool copyFile(const char* leaf){char from[112],to[128];snprintf(from,sizeof from,"/DNSP Readable Logs/Current/%s",leaf);snprintf(to,sizeof to,"%s/%s",outPath,leaf);File a=cardOpen(from,FILE_READ);if(!a)return true;File b=cardOpen(to,FILE_WRITE);if(!b){cardClose(a);return false;}uint8_t buf[512];bool ok=true;while(a.available()){int n=cardRead(a,buf,sizeof buf);if(n<=0||cardWrite(b,buf,n)!=(size_t)n){ok=false;break;}}cardFlush(b);cardClose(a);cardClose(b);return ok;}
int copyCurrentAt(uint16_t wanted){File root=cardOpen("/DNSP Readable Logs/Current");if(!root)return -1;uint16_t seen=0;char leaf[96]{};for(File f=cardNext(root);f;f=cardNext(root)){const char*n=f.name();const char*b=strrchr(n,'/');b=b?b+1:n;if(!f.isDirectory()&&b[0]!='.'&&seen++==wanted){snprintf(leaf,sizeof leaf,"%s",b);cardClose(f);break;}cardClose(f);}cardClose(root);if(!leaf[0])return 0;return copyFile(leaf)?1:-1;}
char sdSourceName[64]{},sdStatePath[128]{};uint32_t sdOffset=0,sdDestSize=0,sdCeiling=0;bool sdOpenError=false;
bool logName(const char* n){const char* b=strrchr(n,'/');b=b?b+1:n;return !strncmp(b,"squachwatch-",12)||!strncmp(b,"dnsp-health",11)||!strncmp(b,"dnsp-research",13)||!strncmp(b,"dnsp-rid-capture",16)||!strcmp(b,"dnsp-field-report.txt")||!strcmp(b,"dnsp-fpv-pit.csv");}
uint8_t sdJournalStep=0;bool sdNeedsCommit=false;
int sdJournalTick(){
 char fresh[132],old[132];snprintf(fresh,sizeof fresh,"%s.new",sdStatePath);snprintf(old,sizeof old,"%s.old",sdStatePath);
 switch(sdJournalStep){
 case 0: if(cardExists(fresh)&&!cardRemove(fresh))return -1;break;
 case 1: {
  char body[112],all[144];int bn=snprintf(body,sizeof body,"DNSP_SD_COPY_V1\noffset=%lu\ndest=%lu\n",(unsigned long)sdOffset,(unsigned long)sdDestSize);
  int n=snprintf(all,sizeof all,"%scrc=%lu\n",body,(unsigned long)Care::crc(body,(size_t)bn));
  File f=cardOpen(fresh,FILE_WRITE);bool ok=f&&cardWrite(f,(const uint8_t*)all,n)==size_t(n);if(f){cardFlush(f);cardClose(f);}if(!ok)return -1;break;
 }
 case 2: if(cardExists(old)&&!cardRemove(old))return -1;break;
 case 3: if(cardExists(sdStatePath)&&!cardRename(sdStatePath,old))return -1;break;
 case 4: if(!cardRename(fresh,sdStatePath))return -1;break;
 case 5: if(cardExists(old)&&!cardRemove(old))return -1;break;
 default: sdJournalStep=0;return 1;
 }
 ++sdJournalStep;return 0;
}
bool saveSdState(){sdJournalStep=0;int q;do{q=sdJournalTick();}while(!q);return q>0;}
bool readSdState(uint32_t& offset,uint32_t& destSize){const char* suffixes[]={"",".new",".old"};bool found=false;for(const char*s:suffixes){char path[136];snprintf(path,sizeof path,"%s%s",sdStatePath,s);File f=cardOpen(path,FILE_READ);if(!f)continue;char b[144]{};size_t n=cardRead(f,(uint8_t*)b,sizeof b-1);cardClose(f);b[n]=0;unsigned long o=0,d=0,c=0;if(DNSP_INTEGER_SCAN(b,"DNSP_SD_COPY_V1\noffset=%lu\ndest=%lu\ncrc=%lu\n",&o,&d,&c)!=3)continue;char body[112];int bn=snprintf(body,sizeof body,"DNSP_SD_COPY_V1\noffset=%lu\ndest=%lu\n",o,d);if(Care::crc(body,(size_t)bn)!=(uint32_t)c)continue;if(!found||o>offset){offset=(uint32_t)o;destSize=(uint32_t)d;found=true;}}return found;}
void sdDestination(char* out,size_t cap){const char* base=strrchr(sdSourceName,'/');snprintf(out,cap,"/DNSP Readable Logs/Current/SD-%s.txt",base?base+1:sdSourceName);for(char* q=strrchr(out,'/')+1;*q;q++)if(*q==' ')*q='_';}
bool openSdLog(uint16_t wanted){
 sdOpenError=false;sdSourceName[0]=0;File root=cardOpen("/");if(!root){sdOpenError=true;return false;}
 uint16_t seen=0;for(File f=cardNext(root);f;f=cardNext(root)){
  const char* n=f.name();if(!f.isDirectory()&&logName(n)&&seen++==wanted){const char* base=strrchr(n,'/');base=base?base+1:n;
   if(strlen(base)+2>sizeof sdSourceName){cardClose(f);cardClose(root);sdOpenError=true;return false;}
   snprintf(sdSourceName,sizeof sdSourceName,"/%s",base);cardClose(f);break;}cardClose(f);
 }cardClose(root);if(!sdSourceName[0])return false;sdOpenError=true;
 snprintf(sdStatePath,sizeof sdStatePath,"/DNSP Readable Logs/Current/.sd-%08lx.state",(unsigned long)hash(sdSourceName,strlen(sdSourceName)));
 sdOffset=sdDestSize=0;bool haveState=readSdState(sdOffset,sdDestSize);
 File source=cardOpen(sdSourceName,FILE_READ);if(!source)return false;sdCeiling=cardSize(source);cardClose(source);
 char dest[128];sdDestination(dest,sizeof dest);File out=cardOpen(dest,FILE_APPEND);if(!out)return false;uint32_t actual=cardSize(out);cardClose(out);
 if(!haveState){sdDestSize=actual;sdOffset=actual<=sdCeiling?actual:0;}
 if(actual<sdDestSize||sdOffset>sdCeiling){return false;}
 if(actual>sdDestSize){
  // A power cut may occur after append but before journal commit. Verify
  // that sole pending chunk; never silently discard mismatching bytes.
  uint32_t delta=actual-sdDestSize;bool same=delta<=512 && delta<=sdCeiling-sdOffset;
  File a=cardOpen(sdSourceName,FILE_READ),b=cardOpen(dest,FILE_READ);same=same&&a&&b&&cardSeek(a,sdOffset)&&cardSeek(b,sdDestSize);
  uint8_t x[64],y[64];uint32_t left=delta;while(same&&left){size_t n=left>sizeof x?sizeof x:left;same=cardRead(a,x,n)==(int)n&&cardRead(b,y,n)==(int)n&&!memcmp(x,y,n);left-=n;}cardClose(a);cardClose(b);
  if(same)sdOffset+=delta;
  else {File c=cardOpen(dest,FILE_APPEND);const char* marker="\n[DNSP: interrupted bytes above retained; source chunk retried]\n";bool ok=c&&cardWrite(c,(const uint8_t*)marker,strlen(marker))==strlen(marker);cardFlush(c);actual=cardSize(c);cardClose(c);if(!ok)return false;}
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
 // Keep the chunk append separate from its durable cursor transaction.
 if(sdNeedsCommit){int q=sdJournalTick();if(q<0)return -1;if(q>0)sdNeedsCommit=false;return 0;}
 if(sdOffset>=sdCeiling){sdSourceName[0]=0;return 1;}
 uint8_t block[512];size_t want=sdCeiling-sdOffset;if(want>sizeof block)want=sizeof block;
 File source=cardOpen(sdSourceName,FILE_READ);bool ok=source&&cardSeek(source,sdOffset)&&cardRead(source,block,want)==(int)want;cardClose(source);if(!ok)return -1;
 char dest[128];sdDestination(dest,sizeof dest);File out=cardOpen(dest,FILE_APPEND);ok=out&&cardSize(out)==sdDestSize&&cardWrite(out,block,want)==want;cardFlush(out);uint32_t actual=cardSize(out);cardClose(out);if(!ok)return -1;
 sdOffset+=want;sdDestSize=actual;sdNeedsCommit=true;sdJournalStep=0;return 0;
}

#endif
}
bool start(Mode m){if(busy())return false;resetTiming("initialize history");autoOwner=false;mode=m;scanAt=newDet=writeAt=bootAt=newBoot=0;lastDet=lastBoot=nextDet=nextBoot=lastRule=nextRule=0;copyIx=sdIx=0;checkpointGeneration=0;activity=0;detLeaf=0;ruleCeiling=0;
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
 finishStep=checkpointStep=0;pendingVerify=false;checkpointCurrent=true;locationSchemaReady=false;commitFrom=IDLE;
#endif
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
 SketchyRule::Incident newestRule{};if(SketchyRule::recent(0,newestRule))ruleCeiling=newestRule.id;
 sdSourceName[0]=0;sdNeedsCommit=false;sdJournalStep=0;BlackBox::captureHistory(history);BlackBox::cursorBegin(detectionCursor);BlackBox::cursorBegin(bootCursor);haveDet=haveBoot=haveRule=false;cardClose(uniqueFile);uniqueActive=false;
 if(!dirs()){p=FAILED;strcpy(msg,"microSD unavailable or folders could not be created.");return false;}// Only a brand-new collection can skip legacy duplicate scans before its first checkpoint.
 const bool freshHistory=!cardExists("/DNSP Readable Logs/Current/SCAN-HISTORY.txt")&&!cardExists("/DNSP Readable Logs/Current/ALL-ALERTS.txt")&&!cardExists("/DNSP Readable Logs/Current/ALERT-HISTORY-v1.5.3.csv")&&!cardExists("/DNSP Readable Logs/Current/SYSTEM-HISTORY.txt");locationSchemaReady=cardExists("/DNSP Readable Logs/Current/.location-schema-v1.2");readCheckpoint();readPending();if(!cardExists("/DNSP Readable Logs/Current/.simulation-schema-v153"))fastRecords=false;if(freshHistory&&!resumedId[0])fastRecords=true;nextDet=lastDet;nextBoot=lastBoot;nextRule=lastRule;
 if(!ensure("ALERT-HISTORY-v1.5.3.csv","boot,uptime_s,type,mac,rssi,channel,hits,flags,vendor,name,location,simulated,address_provenance,record_id\n")||!ensure("ALL-ALERTS.txt","DNSquachWatch stored alert history\n")||!ensure("SCAN-HISTORY.txt","DNSquachWatch stored scan history\n")||!ensure("SYSTEM-HISTORY.txt","DNSquachWatch boot, crash and system history\n")){p=FAILED;strcpy(msg,"Readable files could not be created.");return false;}
 p=FIND_DET;strcpy(msg,"Finding new stored scan events...");strcpy(outPath,"/DNSP Readable Logs/Current");return true;
#else
 p=FAILED;strcpy(msg,"Physical microSD required.");return false;
#endif
}
void tick(){
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
 resetTiming(p==FIND_DET?"find scan record":p==FIND_BOOT?"find boot record":p==COMMIT_RECORD||(p==FINALIZE&&finishStep==2)?(checkpointStep==0?"write checkpoint":checkpointStep==1?"verify checkpoint":"finish checkpoint"):p==WRITE_DET||p==WRITE_BOOT||p==WRITE_RULES?(!liveId[0]?(pendingVerify?"verify pending ID":"prepare pending ID"):uniqueActive?"scan duplicate IDs":"append history record"):p==COPY_SD_LOGS?(sdSourceName[0]?(sdNeedsCommit?"commit copy cursor":"copy log chunk"):"open source log"):"finalize history");
 if(!busy())return;++activity;if(!BlackBox::historyIntact(history)){cardClose(uniqueFile);uniqueActive=false;p=FAILED;strcpy(msg,"Stored history wrapped during export. Retry to capture the new head.");return;}
 if(p==FIND_DET){
  BlackBox::DetRecord r{};auto result=[&](){CardTiming t("flash record read");return BlackBox::nextDetection(history,detectionCursor,r);}();
  if(result==BlackBox::CursorResult::WAIT)return;
  if(result==BlackBox::CursorResult::INVALID){p=FAILED;strcpy(msg,"Could not read captured scan history. Retry backup.");return;}
  uint32_t h=result==BlackBox::CursorResult::RECORD?hash(&r,sizeof r):0;
  if(result==BlackBox::CursorResult::END||(lastDet&&h==lastDet)){
   newDet=scanAt;writeAt=newDet;if(newDet){detectionCursor=lastNewDetection;BlackBox::cursorReverseFromRecord(history.dets,detectionCursor);}p=WRITE_DET;return;
  }
  if(!scanAt)nextDet=h;lastNewDetection=detectionCursor;++scanAt;return;
 }
 if(p==WRITE_DET){
  if(!writeAt){p=FIND_BOOT;strcpy(msg,"Finding new device and system records...");return;}
  if(!haveDet){auto result=[&](){CardTiming t("flash record read");return BlackBox::nextDetection(history,detectionCursor,heldDet);}();if(result==BlackBox::CursorResult::WAIT)return;if(result!=BlackBox::CursorResult::RECORD){p=FAILED;strcpy(msg,"Captured scan record unavailable. Retry backup.");return;}haveDet=true;}
  int q=writeDet(heldDet);if(q<0){p=FAILED;strcpy(msg,"Could not write readable scan history.");}else if(q>0){nextDet=hash(&heldDet,sizeof heldDet);nextBoot=lastBoot;beginCommit(WRITE_DET);}return;
 }
 if(p==FIND_BOOT){
  BlackBox::BootRecord r{};auto result=[&](){CardTiming t("flash record read");return BlackBox::nextBoot(history,bootCursor,r);}();
  if(result==BlackBox::CursorResult::WAIT)return;
  if(result==BlackBox::CursorResult::INVALID){p=FAILED;strcpy(msg,"Could not read captured system history. Retry backup.");return;}
  uint32_t h=result==BlackBox::CursorResult::RECORD?hash(&r,sizeof r):0;
  if(result==BlackBox::CursorResult::END||(lastBoot&&h==lastBoot)){
   newBoot=bootAt;writeAt=newBoot;if(newBoot){bootCursor=lastNewBoot;BlackBox::cursorReverseFromRecord(history.boots,bootCursor);}p=WRITE_BOOT;return;
  }
  if(!bootAt)nextBoot=h;lastNewBoot=bootCursor;++bootAt;return;
 }
 if(p==WRITE_BOOT){
  if(!writeAt){p=WRITE_RULES;strcpy(msg,"Writing rule alerts and export summary...");return;}
  if(!haveBoot){auto result=[&](){CardTiming t("flash record read");return BlackBox::nextBoot(history,bootCursor,heldBoot);}();if(result==BlackBox::CursorResult::WAIT)return;if(result!=BlackBox::CursorResult::RECORD){p=FAILED;strcpy(msg,"Captured system record unavailable. Retry backup.");return;}haveBoot=true;}
  int q=writeBoot(heldBoot);if(q<0){p=FAILED;strcpy(msg,"Could not write readable system history.");}else if(q>0){nextBoot=hash(&heldBoot,sizeof heldBoot);beginCommit(WRITE_BOOT);}return;
 }
 if(p==WRITE_RULES){if(!ensure("SKETCHY-ENVIRONMENT.txt","DNSquachWatch Sketchy Environment rule history\n")){p=FAILED;strcpy(msg,"Could not create rule history.");return;}if(!haveRule){for(uint8_t i=0;i<SketchyRule::count();i++){SketchyRule::Incident candidate{};if(SketchyRule::recent(i,candidate)&&candidate.id>nextRule&&candidate.id<=ruleCeiling&&(!haveRule||candidate.id<heldRule.id)){heldRule=candidate;haveRule=true;}}}if(haveRule){const auto& in=heldRule;if(in.id>nextRule)nextRule=in.id;char a[24],d[24],id[28],line[384];mac(a,sizeof a,in.alpr.mac);mac(d,sizeof d,in.deauth.mac);snprintf(id,sizeof id,"R-%010lu%s",(unsigned long)in.id,SketchyRule::simulationLabel(in)[0]?"-S1":"");snprintf(line,sizeof line,"Incident %lu | %s %s + DEAUTH %s | gap %lus | microSD %s | ALPR location %s | DEAUTH location %s | %s | ALPR simulated %u | DEAUTH simulated %u | ID %s\n",(unsigned long)in.id,detectionTypeName(in.alpr.type),a,d,(unsigned long)in.gapSeconds,in.sdExported?"saved":"pending",LocationLabel::text(in.alpr.locationKey),LocationLabel::text(in.deauth.locationKey),SketchyRule::simulationLabel(in),Simulation::marked(in.alpr.mac),Simulation::marked(in.deauth.mac),id);int q=appendUniqueStep("SKETCHY-ENVIRONMENT.txt",id,line);if(q<0){p=FAILED;strcpy(msg,"Could not write rule history.");return;}if(q>0){beginCommit(WRITE_RULES);}return;}if(nextRule<lastRule)nextRule=lastRule;
  p=FINALIZE;finishStep=checkpointStep=0;return;
 }
 if(p==FINALIZE){
  if(finishStep==0){if(cardExists("/DNSP Readable Logs/Current/EXPORT-SUMMARY.txt")&&!cardRemove("/DNSP Readable Logs/Current/EXPORT-SUMMARY.txt")){p=FAILED;strcpy(msg,"Could not replace export summary; retry is safe.");return;}++finishStep;return;}
  if(finishStep==1){
  char sum[420];snprintf(sum,sizeof sum,"DNSquachWatch readable log collection\nNew scan records this refresh: %u\nNew system records: %u\nStored scan records currently on board: %u\nStored system records currently on board: %u\nRule incidents currently retained: %u\nRecords are ordered by boot number and uptime because this board has no reliable clock. Original data remains unchanged.\n",(unsigned)newDet,(unsigned)newBoot,(unsigned)BlackBox::detectionsKept(),(unsigned)BlackBox::bootsKept(),(unsigned)SketchyRule::count());if(!append("EXPORT-SUMMARY.txt",sum)){p=FAILED;strcpy(msg,"Could not write export summary.");return;}++finishStep;return;}
  if(finishStep==2){int q=checkpointTick();if(q<0){p=FAILED;strcpy(msg,"Refresh checkpoint failed; retry is safe.");return;}if(q>0)++finishStep;return;}
  if(finishStep==3||finishStep==4){if(!ensure(finishStep==3?".cooperative-v121":".simulation-schema-v153","1\n")){p=FAILED;strcpy(msg,"Refresh marker failed; retry is safe.");return;}++finishStep;return;}
  if(mode==Mode::BACKUP_INTERNAL){p=DONE;strcpy(msg,"Internal readable history prepared for backup.");return;}p=COPY_SD_LOGS;strcpy(msg,"Updating readable copies of existing microSD logs...");return;
 }
 if(p==COMMIT_RECORD){
  int q=checkpointTick();if(q<0){p=FAILED;strcpy(msg,"History cursor commit failed; retry is safe.");return;}if(!q)return;
  // Retain the last pending ID rather than deleting/recreating its file.
  // A committed cursor skips it; if the newest cursor is damaged, it still
  // identifies the one record needing duplicate-safe recovery.
  liveId[0]=0;p=commitFrom;
  if(p==WRITE_DET){--writeAt;haveDet=false;}else if(p==WRITE_BOOT){--writeAt;haveBoot=false;}else haveRule=false;
  return;
 }
 if(p==COPY_SD_LOGS){if(sdSourceName[0]){int q=copySdStep();if(q<0){sdSourceName[0]=0;p=FAILED;strcpy(msg,"microSD copy failed; retry retains completed bytes.");return;}if(q>0)++sdIx;return;}if(openSdLog(sdIx))return;if(sdOpenError){sdSourceName[0]=0;p=FAILED;strcpy(msg,"A microSD source or copy journal could not be opened.");return;}
  if(mode==Mode::REFRESH){p=DONE;strcpy(msg,"Readable files refreshed. Duress PIN options are under Security.");return;}
  unsigned n=0;for(unsigned i=1;i<=9999;i++){snprintf(outPath,sizeof outPath,"/DNSP Readable Logs/Exports/Export-%04u",i);if(!cardExists(outPath)){n=i;break;}}if(!n||!cardMkdir(outPath)){p=FAILED;strcpy(msg,"Could not create an organized export folder.");return;}p=COPY_FILES;copyIx=0;strcpy(msg,"Copying organized readable snapshot...");return;
 }
 if(p==COPY_FILES){int q=copyCurrentAt(copyIx);if(q>0){++copyIx;return;}if(q<0){p=FAILED;strcpy(msg,"Organized snapshot copy failed.");return;}p=DONE;strcpy(msg,"Organized snapshot complete. Duress PIN options are under Security.");return;}
#endif
}
void cancel(){if(busy()){
#if defined(ARDUINO_ARCH_ESP32) || defined(READABLE_LOGS_TEST)
 sdSourceName[0]=0;cardClose(uniqueFile);uniqueActive=false;
#endif
 p=FAILED;strcpy(msg,"Readable-log operation cancelled; completed files were kept.");}}
bool automatic(){return autoOwner;}
void automaticTick(uint32_t now,bool available,bool mayWork){
 static uint32_t last=0,lastWork=0;
 // Automatic readable copies can lag raw event storage. Leave display/input
 // passes between card steps; manual refresh and backup do not use this gate.
 if(autoOwner&&busy()){if(!available||!Settings::autoHistory()){cancel();autoOwner=false;return;}if(mayWork&&now-lastWork>=250u){lastWork=now;tick();}return;}
 if(!busy())autoOwner=false;
 if(!available||!mayWork||!Settings::autoHistory()||busy()||now-last<60000u)return;
 last=lastWork=now;autoOwner=start(Mode::REFRESH);
}
bool busy(){return p>=FIND_DET&&p<=COPY_FILES;}unsigned percent(){if(p==COMMIT_RECORD)return commitFrom==WRITE_DET?(10+(BlackBox::detectionsKept()?40u*(newDet-writeAt)/BlackBox::detectionsKept():40)):commitFrom==WRITE_BOOT?(55+(BlackBox::bootsKept()?20u*(newBoot-writeAt)/BlackBox::bootsKept():20)):78;if(p==FINALIZE)return 80;if(p==DONE)return 100;if(!busy())return 0;if(p<=WRITE_DET)return 10+(BlackBox::detectionsKept()?40u*(p==FIND_DET?scanAt:newDet-writeAt)/BlackBox::detectionsKept():40);if(p<=WRITE_BOOT)return 55+(BlackBox::bootsKept()?20u*(p==FIND_BOOT?bootAt:newBoot-writeAt)/BlackBox::bootsKept():20);if(p==WRITE_RULES)return 78;if(p==COPY_SD_LOGS)return 82;if(p==COPY_FILES){unsigned q=84u+2u*copyIx;return q>99?99:q;}return 0;}
uint32_t progressToken(){return activity;}
bool succeeded(){return p==DONE;}
const char* operation(){return lastOperation;}uint32_t operationMicros(){return lastOperationUs;}const char* step(){return lastStep;}
const char* phase(){Phase shown=p==COMMIT_RECORD?commitFrom:p;return shown<=WRITE_DET?"SCAN LOGS":shown<=WRITE_BOOT?"SYSTEM LOGS":(shown==WRITE_RULES||shown==FINALIZE)?"RULE LOGS":p==COPY_SD_LOGS?"MICROSD LOGS":p==COPY_FILES?"ORGANIZING":"READY";}const char* status(){return msg;}const char* outputPath(){return outPath;}
}
