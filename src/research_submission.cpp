#include "research_submission.h"
#include <cstdio>
#include <cstring>
namespace ResearchSubmission {
namespace {
// JSON strings escape controls, quotes and backslashes, including user input.
bool quote(char* out,size_t cap,const char* text,size_t limit){
    size_t n=0;if(cap<3)return false;out[n++]='"';
    for(size_t i=0;i<limit&&text[i];++i){unsigned char c=text[i];char escaped[7];const char* add=escaped;size_t k;
        if(c<32||c>=127){snprintf(escaped,sizeof escaped,"\\u%04x",c);k=6;}
        else if(c=='"'||c=='\\'){escaped[0]='\\';escaped[1]=c;k=2;}
        else {escaped[0]=c;k=1;}
        if(n+k+2>cap)return false;memcpy(out+n,add,k);n+=k;
    }
    out[n++]='"';out[n]=0;return true;
}
}
bool format(char* out,size_t cap,const UserLabels::Target& t,const UserLabels::Label& l,uint32_t uptime,bool identifiers){
    if(!out||!cap)return false;out[0]=0;
    char sub[153],name[147],tag[147],detected[147];
    char safeSub[sizeof l.subtag];memcpy(safeSub,l.subtag,sizeof safeSub);safeSub[sizeof safeSub-1]=0;
    if(!identifiers){for(size_t i=0;i+17<=strlen(safeSub);++i){bool mac=true;for(size_t j=0;j<17;++j){char c=safeSub[i+j];if(j%3==2){if(c!=':'&&c!='-')mac=false;}else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))mac=false;}if(mac)for(size_t j=9;j<17;++j)if(j%3!=2)safeSub[i+j]='X';}}
    if(!quote(sub,sizeof sub,safeSub,sizeof safeSub)||!quote(name,sizeof name,identifiers?t.name:"",sizeof t.name)||!quote(tag,sizeof tag,UserLabels::typeName(l.type),24)||!quote(detected,sizeof detected,detectionTypeName(t.original),24))return false;
    char mac[24];
    if(identifiers)snprintf(mac,sizeof mac,"%02X:%02X:%02X:%02X:%02X:%02X",t.mac[0],t.mac[1],t.mac[2],t.mac[3],t.mac[4],t.mac[5]);
    else snprintf(mac,sizeof mac,"%02X:%02X:%02X:XX:XX:XX",t.mac[0],t.mac[1],t.mac[2]);
    const int n=snprintf(out,cap,
        "{\n\"schema\":\"dnsp-device-research-v1\",\n\"firmware\":\"DNSquachWatch v1.5\",\n"
        "\"status\":\"unverified\",\n\"scope\":\"individual-device-observation\",\n"
        "\"radio\":\"%s\",\n\"observed_mac\":\"%s\",\n\"observed_prefix\":\"%02X:%02X:%02X\",\n"
        "\"address_assessment\":\"%s\",\n\"identifiers_included\":%s,\n\"advertised_name_bytes\":%s,\n"
        "\"detected_tag\":%s,\n\"user_label\":%s,\n\"subtag\":%s,\n"
        "\"signature_id\":%u,\n\"match_evidence_id\":%u,\n\"detector_confidence_id\":%u,\n"
        "\"rssi_dbm\":%d,\n\"channel\":%u,\n\"uptime_ms\":%lu,\n"
        "\"caution\":\"User labels and detector confidence do not confirm identity. Prefixes may be shared; MACs may be randomized or spoofed. BLE address type is unknown.\",\n"
        "\"submit_to\":\"https://github.com/your-dnsp/DNSquachWatch/issues/new?template=device_research.yml\"\n}\n",
        t.ble?"BLE":"Wi-Fi",mac,t.mac[0],t.mac[1],t.mac[2],t.ble?"BLE public/random type unknown":(t.mac[0]&1)?"multicast/invalid individual address":(t.mac[0]&2)?"locally administered; not vendor OUI proof":"universally administered; prefix not identity proof",
        identifiers?"true":"false",name,detected,tag,sub,t.signature,(unsigned)t.evidence,(unsigned)t.confidence,t.rssi,t.channel,(unsigned long)uptime);
    if(n<0||(size_t)n>=cap){out[0]=0;return false;}return true;
}
}
#if defined(ARDUINO_ARCH_ESP32) || defined(RESEARCH_SUBMISSION_TEST)
#include <SD.h>
namespace ResearchSubmission {
namespace {
bool folders(){return SD.cardSize()&&(SD.exists("/Research Submissions")||SD.mkdir("/Research Submissions"))&&(SD.exists("/Research Submissions/PRIVATE")||SD.mkdir("/Research Submissions/PRIVATE"));}
bool reportPath(char* out,size_t cap,uint32_t now,unsigned id,bool identifiers){int n=snprintf(out,cap,"/Research Submissions/%sreport-%08lx-%04u-%s.txt",identifiers?"PRIVATE/":"",(unsigned long)now,id,identifiers?"PRIVATE":"REDACTED");return n>0&&(size_t)n<cap;}
bool publish(const UserLabels::Target& t,const UserLabels::Label& l,uint32_t now,bool identifiers,char* path){
 char body[1792];if(!format(body,sizeof body,t,l,now,identifiers))return false;
 char tmp[112];int n=snprintf(tmp,sizeof tmp,"%s.pending",path);if(n<=0||n>=(int)sizeof tmp)return false;SD.remove(tmp);
 File f=SD.open(tmp,FILE_WRITE);if(!f)return false;const size_t bytes=strlen(body);bool ok=f.write((const uint8_t*)body,bytes)==bytes;f.flush();f.close();
 f=SD.open(tmp,FILE_READ);ok=ok&&f&&f.size()==bytes;uint8_t check[96];size_t off=0;
 while(ok&&off<bytes){size_t want=bytes-off;if(want>sizeof check)want=sizeof check;ok=f.read(check,want)==(int)want&&!memcmp(check,body+off,want);off+=want;}f.close();
 if(ok)ok=SD.rename(tmp,path);if(!ok)SD.remove(tmp);return ok;
}
}
bool save(const UserLabels::Target& t,const UserLabels::Label& l,uint32_t now,bool identifiers,char* out,size_t cap){
 if(!out||!cap)return false;out[0]=0;if(!folders())return false;
 for(unsigned id=1;id<=9999;++id){if(!reportPath(out,cap,now,id,identifiers)){out[0]=0;return false;}if(SD.exists(out))continue;if(publish(t,l,now,identifiers,out))return true;out[0]=0;return false;}out[0]=0;return false;
}
bool savePair(const UserLabels::Target& t,const UserLabels::Label& l,uint32_t now,char* out,size_t cap){
 if(!out||!cap)return false;out[0]=0;if(!folders())return false;
 char privatePath[96];
 // Allocate one collision-safe ID for BOTH files. Publish private first,
 // redacted last; only a fully checked pair yields success to the UI.
 for(unsigned id=1;id<=9999;++id){if(!reportPath(privatePath,sizeof privatePath,now,id,true)||!reportPath(out,cap,now,id,false)){out[0]=0;return false;}
  if(SD.exists(privatePath)||SD.exists(out))continue;
  if(!publish(t,l,now,true,privatePath)){out[0]=0;return false;}
  if(publish(t,l,now,false,out))return true;
  SD.remove(privatePath);out[0]=0;return false;
 }out[0]=0;return false;
}
}
#else
namespace ResearchSubmission {
bool save(const UserLabels::Target&,const UserLabels::Label&,uint32_t,bool,char* out,size_t cap){if(out&&cap)out[0]=0;return false;}
bool savePair(const UserLabels::Target&,const UserLabels::Label&,uint32_t,char* out,size_t cap){if(out&&cap)out[0]=0;return false;}
}
#endif
