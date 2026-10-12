#include "firmware_version.h"
#include "backup_maintenance.h"
#include <cstdio>
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32) || defined(BACKUP_MAINTENANCE_TEST)
#include <SD.h>
#include <Arduino.h>
#endif
namespace BackupMaintenance {
static bool running=false,archive=false;static unsigned slot=0,moved=0,skipped=0;static char msg[96]="Archive older complete backups or remove incomplete backups.";
const char* status(){return msg;}bool busy(){return running;}
#if defined(ARDUINO_ARCH_ESP32) || defined(BACKUP_MAINTENANCE_TEST)
static File folder;static unsigned leaves=0,archiveIndex=0;static bool deleting=false;
// Read only the release numbers we need. scanf pulls the general-purpose
// floating-point scanner into an otherwise integer-only archive decision.
// Reject overflow and malformed versions so uncertain backups stay put.
static bool versionParts(const char* s, unsigned (&v)[3]) {
 if (*s=='v') ++s;
 for (unsigned i=0;i<3;++i) {
  if (*s<'0'||*s>'9') return false;
  unsigned n=0;
  do { const unsigned digit=*s++-'0'; if(n>6553||(n==6553&&digit>5))return false;n=n*10+digit; } while(*s>='0'&&*s<='9');
  v[i]=n;
  if(i<2) { if(*s=='.'){++s;continue;} if(i==1&&(*s=='\n'||*s=='\r'||!*s)){v[2]=0;return true;} return false; }
 }
 return !*s||*s=='\n'||*s=='\r';
}
static bool older(const char* version){
 unsigned a[3]{},b[3]{};
 if(!versionParts(version,a)||!versionParts(FIRMWARE_VERSION,b))return false;
 for(unsigned i=0;i<3;++i){if(a[i]!=b[i])return a[i]<b[i];}return false;
}

static char root[32];
bool start(bool old,bool mounted){if(running||!mounted)return false;running=true;archive=old;slot=moved=skipped=0;leaves=archiveIndex=0;deleting=false;snprintf(msg,sizeof msg,"%s... Keep power connected.",old?"Archiving older backups":"Removing incomplete backups");return true;}
void tick(){
 if(!running)return;
 if(slot>=10){folder.close();running=false;snprintf(msg,sizeof msg,"%u %s; %u skipped. Completed backups preserved.",moved,archive?"archived in /Archive":"incomplete slots removed",skipped);return;}
 snprintf(root,sizeof root,"/dnsp-backup-%u",slot);
 if(folder){
  File f=folder.openNextFile();
  if(f){char name[128];snprintf(name,sizeof name,"%s",f.name());bool nested=f.isDirectory();f.close();if(nested||++leaves>64){folder.close();++skipped;++slot;return;}
   const char* leaf=strrchr(name,'/');leaf=leaf?leaf+1:name;
   if(!leaf[0]||strchr(leaf,'\\')||!strcmp(leaf,".")||!strcmp(leaf,"..")){folder.close();++skipped;++slot;return;}
   if(!deleting)return;
   char path[96];snprintf(path,sizeof path,"%s/%s",root,leaf);if(strlen(root)+1+strlen(leaf)>=sizeof path||!SD.remove(path)){folder.close();++skipped;++slot;}return;
  }
  folder.close();if(!deleting){folder=SD.open(root,FILE_READ);deleting=true;leaves=0;return;}if(SD.rmdir(root))++moved;else ++skipped;++slot;return;
 }
 if(!SD.exists(root)){++slot;return;}
 char complete[48];snprintf(complete,sizeof complete,"%s/COMPLETE.txt",root);
 if(archive){
  File f=SD.open(complete,FILE_READ);char b[112]{};if(f)f.read((uint8_t*)b,sizeof b-1);f.close();const char* v=strstr(b,"firmware=DNSquachWatch ");
  if(!v||!older(v+strlen("firmware=DNSquachWatch "))){++skipped;++slot;return;}
  if(!SD.exists("/Archive")&&!SD.mkdir("/Archive")){cancel();strcpy(msg,"Cannot create /Archive. Backups unchanged.");return;}
  char dest[64];snprintf(dest,sizeof dest,"/Archive/dnsp-backup-%u-%04u",slot,archiveIndex);
  if(SD.exists(dest)&&archiveIndex<9999){++archiveIndex;return;}
  if(!SD.exists(dest)&&SD.rename(root,dest))++moved;else ++skipped;archiveIndex=0;++slot;return;
 }
 if(SD.exists(complete)){++skipped;++slot;return;}folder=SD.open(root,FILE_READ);leaves=0;deleting=false;if(!folder||!folder.isDirectory()){folder.close();++skipped;++slot;}
}
void cancel(){folder.close();running=false;strcpy(msg,"Maintenance stopped. Retry to finish remaining slots.");}
#else
bool start(bool,bool){return false;}void tick(){}void cancel(){running=false;}
#endif
}
