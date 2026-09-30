#include "crash_reports.h"
#include <Preferences.h>
#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <esp_heap_caps.h>
namespace CrashReports {
static bool drained=false;
static const char* message="No report export pending.";
const char* status(){return message;}
const char* root(){
#if defined(ARDUINO_ARCH_ESP32)
    return "/sd";
#else
    const char* p=getenv("DNSP_TEST_SD");return p&&*p?p:"/nonexistent-dnsp-sd";
#endif
}
static void key(char* out,unsigned i){snprintf(out,8,"r%u",i);}
bool enqueue(const Record& input){
    Preferences p;if(!p.begin("crashqueue",false))return false;
    unsigned slot=8;char k[8];
    for(unsigned i=0;i<8;++i){key(k,i);if(!p.getBytesLength(k)){slot=i;break;}}
    if(slot==8){p.end();message="Crash queue full (8); earlier reports retained.";return false;}
    Record r=input;r.firmware[sizeof r.firmware-1]=0;
    r.sequence=p.getUInt("seq",0)+1;if(!r.sequence)r.sequence=1;
    p.putUInt("seq",r.sequence);
    bool ok=p.getUInt("seq",0)==r.sequence;
    drained=false;
    key(k,slot);ok=ok&&p.putBytes(k,&r,sizeof r)==sizeof r;p.end();
    message=ok?"Crash queued; export after SD is ready.":"Could not preserve crash in flash.";return ok;
}
static bool regular(const char* path){struct stat st;return stat(path,&st)==0&&S_ISREG(st.st_mode)&&st.st_size>0;}
bool backupPresent(bool ready){
    if(!ready)return false;char path[160];
    for(unsigned i=0;i<10;++i){snprintf(path,sizeof path,"%s/dnsp-backup-%u/COMPLETE.txt",root(),i);if(!regular(path))continue;
        snprintf(path,sizeof path,"%s/dnsp-backup-%u/firmware.bin",root(),i);if(regular(path))return true;}
    return false;
}
void service(bool ready,uint32_t now){
    static uint32_t last=0;if(drained||!ready||now-last<5000)return;last=now;
    if(heap_caps_get_free_size(MALLOC_CAP_8BIT)<16384 || heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)<8192){message="Crash export waiting for memory.";return;}
    Preferences p;if(!p.begin("crashqueue",false))return;
    Record r;char k[8];bool found=false;
    for(unsigned i=0;i<8;++i){key(k,i);if(p.getBytesLength(k)==sizeof r && p.getBytes(k,&r,sizeof r)==sizeof r){found=true;break;}}
    p.end();if(!found){drained=true;return;}
    r.firmware[sizeof r.firmware-1]=0;r.crash.task[sizeof r.crash.task-1]=0;
    // Stable ID permits a retry after rename but before NVS removal without duplicate reports.
    uint32_t hash=2166136261u;const uint8_t* bytes=(const uint8_t*)&r;
    for(size_t i=0;i<sizeof r;++i)hash=(hash^bytes[i])*16777619u;
    char dir[128],path[176],tmp[184];snprintf(dir,sizeof dir,"%s/crash-reports",root());
    if(mkdir(dir,0770)!=0&&errno!=EEXIST){message="Crash export: cannot create folder.";return;}
    struct stat ds;if(stat(dir,&ds)!=0||!S_ISDIR(ds.st_mode)){message="Crash export: cannot create folder.";return;}
    snprintf(path,sizeof path,"%s/crash-%08lx-%08lx.txt",dir,(unsigned long)r.sequence,(unsigned long)hash);
    if(!regular(path)){
        snprintf(tmp,sizeof tmp,"%s.partial",path);FILE* f=fopen(tmp,"wb");
        if(!f){message="Crash export: cannot open file.";return;}
        int n=fprintf(f,"DNSquachWatch crash report\nReport ID: %lu\nFirmware at recovery: %s\nReset reason code: %lu\nDate: unknown unless independently recorded\nPrevious uptime ms: %lu\nHeap bytes free: %lu\nLargest heap block: %lu\nScreen ID: %u\nBreadcrumb valid: %s\nCore dump: %s\nDump from older firmware: %s\nTask: %s\nPC: %08lx\nException cause: %lu\nFault address: %08lx\nBacktrace: %08lx %08lx %08lx %08lx\nLast light reading: %u\nLDR enabled: %s\nLast backlight duty: %u/255\nDisplay: ST7789-%uMHz\nThese observations do not establish the cause.\n",(unsigned long)r.sequence,r.firmware,(unsigned long)r.resetReason,(unsigned long)r.crash.uptimeMs,(unsigned long)r.crash.heapFree,(unsigned long)r.crash.heapBlock,r.crash.screen,r.crash.valid?"yes":"no",r.crash.haveDump?"yes":"no",r.crash.dumpOlder?"yes":"no",r.crash.task,(unsigned long)r.crash.pc,(unsigned long)r.crash.cause,(unsigned long)r.crash.vaddr,(unsigned long)r.crash.bt[0],(unsigned long)r.crash.bt[1],(unsigned long)r.crash.bt[2],(unsigned long)r.crash.bt[3],r.lightReading,r.ldr?"yes":"no",r.backlightDuty,r.displayMhz);
        bool ok=n>0&&!ferror(f);ok=(fflush(f)==0)&&ok;ok=(fsync(fileno(f))==0)&&ok;ok=(fclose(f)==0)&&ok;
        if(!ok||rename(tmp,path)!=0){message="Crash export failed; queued record retained.";return;}
    }
    if(p.begin("crashqueue",false)){bool ok=p.remove(k);p.end();message=ok?"Crash report saved in /crash-reports/.":"Report saved; queue cleanup pending.";}
}
}
