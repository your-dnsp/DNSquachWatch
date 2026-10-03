#include "research.h"
#include <cstdio>
#include <cstdarg>
namespace Research {
bool formatReport(const Stats& s,char* out,size_t cap){
 if(!out||!cap)return false;size_t at=0;bool ok=true;
 auto add=[&](const char* fmt,...){if(!ok)return;va_list args;va_start(args,fmt);int n=vsnprintf(out+at,cap-at,fmt,args);va_end(args);if(n<0||size_t(n)>=cap-at){ok=false;return;}at+=n;};
 add("DNSP FIELD REPORT - v1\nSession %lu; catalog %lu; profile %s\nStatus: %s; elapsed %lu seconds (uptime, not calendar time)\n",
 (unsigned long)s.session,(unsigned long)s.catalog,profileName(s.profile),s.active?"IN PROGRESS":"STOPPED",(unsigned long)(s.elapsed/1000));
 add("Observed packets: %lu\nSaved records (includes annotations): %lu\nOmitted records: %lu\nWrite errors: %lu\nUnderlying capture: %s\n",
 (unsigned long)s.observed,(unsigned long)s.saved,(unsigned long)s.dropped,(unsigned long)s.errors,s.raw?"RAW - contains identifiers":"REDACTED");
 add("DEAUTH frames saved: %lu; coherent per-source bursts: %lu; multi-target bursts: %lu\nProtected / unprotected DEAUTH frames: %lu / %lu; readable reason codes: %lu\n",
 (unsigned long)s.deauthFrames,(unsigned long)s.deauthBursts,(unsigned long)s.deauthMultiTargetBursts,(unsigned long)s.deauthProtected,(unsigned long)s.deauthUnprotected,(unsigned long)s.deauthReasonKnown);
 add("\nSaved observations by radio classification (not unique devices):\nType: low / medium / high confidence\n");
 add("SIMULATED observations: %lu. DNSP zero-suffix test convention; not reserved or authenticated.\n",(unsigned long)s.simulated);
 for(unsigned i=0;i<19;i++)if(s.types[i][0]||s.types[i][1]||s.types[i][2])add("%s: %lu / %lu / %lu\n",detectionTypeName((DetectionType)i),(unsigned long)s.types[i][0],(unsigned long)s.types[i][1],(unsigned long)s.types[i][2]);
 add("\nCoverage: BLE enabled %lu seconds; WiFi enabled %lu seconds.\nWiFi channel enabled time (seconds):",(unsigned long)(s.bleMs/1000),(unsigned long)(s.wifiMs/1000));
 for(int i=1;i<=13;i++)if(s.channels&(1u<<i))add(" %d=%lu",i,(unsigned long)(s.channelMs[i]/1000));
 add("\nEnabled time is not measured airtime. A shared 2.4GHz radio can miss transmissions during channel hopping. DEAUTH counts represent only frames observed during channel dwell periods.\n\nEvidence and limits:\nConfidence describes rule strength, not a calibrated probability. Manufacturer prefixes alone do not confirm a product. Names and radio identifiers can be spoofed. A DEAUTH transmitter address is a claimed frame value, not authenticated hardware identity. Protected or unprotected status alone does not prove an attack. See per-record evidence/rule/confidence in the accompanying JSONL/CSV; field notes are separate from radio confidence.\nRepeated packets are not a device count. Address changes prevent reliable deduplication. No detection does not mean no camera. Meta clues do not reveal recording status. FPV setup clues do not establish flight. Wired/cellular cameras and 5.8GHz video can be invisible.\nThis report contains no MAC addresses, payloads, names, coordinates or free-text notes. Raw capture files may contain private identifiers.\n");
 return ok;
}
}
