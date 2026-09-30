#include "user_labels.h"
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32)
#include <Preferences.h>
#endif

namespace UserLabels {
namespace {
struct Entry { bool used=false; uint8_t mac[6]{}; Label label{}; uint32_t order=0; };
Entry entries[16];
uint32_t sequence=0;
ExportSink exportSink=nullptr;
void persist(){
#if defined(ARDUINO_ARCH_ESP32)
    Preferences p;if(p.begin("dnsp-labels",false)){p.putBytes("entries",entries,sizeof entries);p.putULong("seq",sequence);p.end();}
#endif
}
}

void begin(){
#if defined(ARDUINO_ARCH_ESP32)
    Preferences p;if(p.begin("dnsp-labels",true)){if(p.getBytesLength("entries")==sizeof entries)p.getBytes("entries",entries,sizeof entries);sequence=p.getULong("seq",0);p.end();}
#endif
}

void setExportSink(ExportSink sink){exportSink=sink;}

const char* typeName(uint8_t type){
    return type==OTHER_TAG?"OTHER":detectionTypeName((DetectionType)type);
}

bool lookup(const uint8_t mac[6],Label& out){
    if(!mac)return false;
    for(const auto& e:entries)if(e.used&&!memcmp(e.mac,mac,6)){out=e.label;return true;}
    return false;
}

bool save(const Target& target,const Label& label,uint32_t now){
    if((label.type==OTHER_TAG&&!label.subtag[0]) ||
       (label.type!=OTHER_TAG&&(label.type==0||label.type>=(uint8_t)DetectionType::COUNT)))return false;
    // A label is only committed after its required microSD export succeeds.
    if(!exportSink||!exportSink(target,label,now))return false;
    Entry* slot=nullptr;
    for(auto& e:entries)if(e.used&&!memcmp(e.mac,target.mac,6)){slot=&e;break;}
    if(!slot)for(auto& e:entries)if(!e.used){slot=&e;break;}
    if(!slot){slot=&entries[0];for(auto& e:entries)if(e.order<slot->order)slot=&e;}
    slot->used=true;memcpy(slot->mac,target.mac,6);slot->label=label;slot->order=++sequence;
    persist();
    return true;
}

uint8_t count(){uint8_t n=0;for(const auto&e:entries)if(e.used)++n;return n;}
bool at(uint8_t index,uint8_t mac[6],Label& out){for(const auto&e:entries)if(e.used){if(index){--index;continue;}memcpy(mac,e.mac,6);out=e.label;return true;}return false;}
bool restore(const uint8_t mac[6],const Label& label){if(!mac||(label.type==OTHER_TAG&&!label.subtag[0]))return false;Entry* s=nullptr;for(auto&e:entries)if(e.used&&!memcmp(e.mac,mac,6)){s=&e;break;}if(!s)for(auto&e:entries)if(!e.used){s=&e;break;}if(!s)return false;s->used=true;memcpy(s->mac,mac,6);s->label=label;s->order=++sequence;persist();return true;}
void clearAll(){memset(entries,0,sizeof entries);sequence=0;persist();}

void clearSession(){memset(entries,0,sizeof entries);sequence=0;}

} // namespace UserLabels
