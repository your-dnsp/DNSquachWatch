#include "research.h"
#include "drone_watch.h"
#if defined(ARDUINO_ARCH_ESP32)
#include <SD.h>
#include <cstdlib>
namespace Research {
namespace {
const char* paths[]={"/dnsp-research.jsonl","/dnsp-research.csv","/dnsp-research.previous.jsonl","/dnsp-research.previous.csv"};
bool append(const char* path,const char* line){File f=SD.open(path,FILE_APPEND);if(!f)return false;size_t n=strlen(line);bool ok=f.write((const uint8_t*)line,n)==n;f.flush();f.close();return ok;}
}
bool storageSink(const char* json,const char* csv,bool reset){
    if(reset){
        for(int i=0;i<2;i++){if(SD.exists(paths[i])){if(SD.exists(paths[i+2])&&!SD.remove(paths[i+2]))return false;if(!SD.rename(paths[i],paths[i+2]))return false;}}
        return append(paths[0],"{\"kind\":\"session\",\"schema\":2,\"warning\":\"Experimental matches. Absence is not clearance. Raw mode includes identifiers. DEAUTH addresses are claimed frame values and may be spoofed.\"}\n")&&append(paths[1],"session,id,reference,uptime_ms,catalog,radio,mac,rssi,channel,type,evidence,rule,verdict,confidence,claimed_transmitter,receiver,bssid,deauth_reason,management_protection,location\n");
    }
    return append(paths[0],json)&&append(paths[1],csv);
}
bool storageReport(const Stats& stats){
    char report[3072];if(!formatReport(stats,report,sizeof report))return false;
    const char* partial="/dnsp-field-report.partial", *current="/dnsp-field-report.txt", *previous="/dnsp-field-report.previous.txt";
    File f=SD.open(partial,FILE_WRITE);if(!f)return false;size_t n=strlen(report);bool ok=f.write((const uint8_t*)report,n)==n;f.flush();f.close();if(!ok)return false;
    // Keep the previous report until a complete replacement exists.
    if(SD.exists(current)){if(SD.exists(previous)&&!SD.remove(previous))return false;if(!SD.rename(current,previous))return false;}
    return SD.rename(partial,current);
}
bool storageImport(){
    File f=SD.open("/dnsp-signatures.txt",FILE_READ);if(!f){stop("No /dnsp-signatures.txt on microSD");return false;}
    if(f.size()>PACK_CAP){f.close();stop("Signature file too large (4096 max)");return false;}
    const size_t want=f.size();
    char* data=(char*)malloc(want ? want : 1);
    if(!data){f.close();stop("Not enough memory to import pack");return false;}
    const size_t n=f.readBytes(data,want);f.close();
    if(n!=want){free(data);stop("Signature file read failed");return false;}
    bool ok=importPack(data,n);free(data);return ok;
}
void storageWipe(){DroneWatch::wipe();discard();stop("Research stopped for history wipe");for(const char* p:paths)SD.remove(p);SD.remove("/dnsp-field-report.txt");SD.remove("/dnsp-field-report.previous.txt");SD.remove("/dnsp-field-report.partial");}
}
#else
namespace Research {
bool storageSink(const char*,const char*,bool){return false;}
bool storageReport(const Stats&){return false;}
bool storageImport(){stop("Emulator: use parser tests for SD imports");return false;}
void storageWipe(){DroneWatch::wipe();discard();stop("Research stopped for history wipe");}
}
#endif
