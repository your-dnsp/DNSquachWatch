#include "user_labels.h"
#include "csv_text.h"
#if defined(ARDUINO_ARCH_ESP32)
#include <SD.h>
#include <cstdio>
#include <cstring>

namespace UserLabels {
namespace {
constexpr const char* DIR="/User Labeled Device Findings";
constexpr const char* CSV="/User Labeled Device Findings/user-labeled-device-findings.csv";

const char* evidenceName(MatchEvidence e){
    switch(e){case MatchEvidence::OUI:return "OUI";case MatchEvidence::SSID:return "SSID";case MatchEvidence::BLE_COMPANY:return "BLE company";case MatchEvidence::BLE_SERVICE:return "BLE service";case MatchEvidence::BLE_NAME:return "BLE name";case MatchEvidence::FIND_MY:return "Find My";case MatchEvidence::IBEACON:return "iBeacon";case MatchEvidence::DEAUTH_BURST:return "deauth burst";case MatchEvidence::EVIL_TWIN:return "evil twin";case MatchEvidence::PWNAGOTCHI:return "Pwnagotchi";case MatchEvidence::RESEARCH_COMPOSITE:return "research composite";case MatchEvidence::WIFI_REMOTE_ID:return "WiFi Remote ID";case MatchEvidence::BLE_REMOTE_ID:return "BLE Remote ID";default:return "unknown";}
}
const char* confidenceName(Confidence c){return c==Confidence::HIGH_CONF?"high":c==Confidence::MED_CONF?"medium":"low";}
void macText(char* out,size_t n,const uint8_t* m,char sep=':'){snprintf(out,n,"%02X%c%02X%c%02X%c%02X%c%02X%c%02X",m[0],sep,m[1],sep,m[2],sep,m[3],sep,m[4],sep,m[5]);}
bool writeAll(File& f,const char* text){const size_t n=strlen(text);return f.write((const uint8_t*)text,n)==n;}
}

bool storageExport(const Target& target,const Label& label,uint32_t now){
    if(!SD.cardSize())return false;
    if(!SD.exists(DIR)&&!SD.mkdir(DIR))return false;
    char mac[24],leafMac[24],path[96];macText(mac,sizeof mac,target.mac);macText(leafMac,sizeof leafMac,target.mac,'-');
    snprintf(path,sizeof path,"%s/%s-%lu.txt",DIR,leafMac,(unsigned long)now);
    File one=SD.open(path,FILE_WRITE);if(!one)return false;
    char body[768];
    snprintf(body,sizeof body,
        "DNSquachWatch user-labeled device finding\n"
        "Saved uptime ms: %lu\nOUI: %02X:%02X:%02X\nMAC: %s\nRadio: %s\n"
        "Detected tag: %s\nUser label: %s\nSubtag: %s\n"
        "Device name: %s\nVendor: %s\nRSSI: %d dBm\nChannel: %u\n"
        "Evidence: %s\nSignature: 0x%04X\nConfidence: %s\n"
        "A user label records an observation; it does not prove identity, ownership, function, or intent.\n",
        (unsigned long)now,target.mac[0],target.mac[1],target.mac[2],mac,target.ble?"BLE":"Wi-Fi",
        detectionTypeName(target.original),typeName(label.type),label.subtag[0]?label.subtag:"(none)",
        target.name[0]?target.name:"(not advertised)",target.vendor[0]?target.vendor:"(unknown)",target.rssi,target.channel,
        evidenceName(target.evidence),target.signature,confidenceName(target.confidence));
    bool ok=writeAll(one,body);one.flush();one.close();if(!ok)return false;

    const bool newCsv=!SD.exists(CSV);
    File csv=SD.open(CSV,FILE_APPEND);if(!csv)return false;
    if(newCsv&&!writeAll(csv,"uptime_ms,oui,device_name,mac,detected_tag,user_label,subtag,radio,rssi,channel,vendor,evidence,confidence\n")){csv.close();return false;}
    char safeName[32],safeSub[32],safeVendor[32];
    safeCsvText(safeName,sizeof safeName,target.name,sizeof target.name);
    safeCsvText(safeSub,sizeof safeSub,label.subtag,sizeof label.subtag);
    safeCsvText(safeVendor,sizeof safeVendor,target.vendor,sizeof target.vendor);
    char row[384];snprintf(row,sizeof row,"%lu,%02X:%02X:%02X,%s,%s,%s,%s,%s,%s,%d,%u,%s,%s,%s\n",
        (unsigned long)now,target.mac[0],target.mac[1],target.mac[2],safeName,mac,
        detectionTypeName(target.original),typeName(label.type),safeSub,target.ble?"BLE":"Wi-Fi",target.rssi,target.channel,
        safeVendor,evidenceName(target.evidence),confidenceName(target.confidence));
    ok=writeAll(csv,row);csv.flush();csv.close();return ok;
}

void storageWipe(){
    if(!SD.cardSize()||!SD.exists(DIR))return;
    // Delete in small fixed-memory batches until the directory is empty. A
    // long-running field project can create far more than 24 finding files;
    // stopping after one batch would leave user observations behind during a
    // duress wipe.
    for(uint16_t pass=0;pass<512;pass++){
        File dir=SD.open(DIR);if(!dir)return;char paths[12][112];uint8_t n=0;
        for(File f=dir.openNextFile();f&&n<12;f=dir.openNextFile()){const char* name=f.name();snprintf(paths[n++],sizeof paths[0],"%s/%s",DIR,strrchr(name,'/')?strrchr(name,'/')+1:name);f.close();}
        dir.close();if(!n)break;for(uint8_t i=0;i<n;i++)SD.remove(paths[i]);
    }
    SD.rmdir(DIR);
}
} // namespace UserLabels
#else
namespace UserLabels { bool storageExport(const Target&,const Label&,uint32_t){return false;} void storageWipe(){} }
#endif
