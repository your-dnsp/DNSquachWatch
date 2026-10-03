#include "location_label.h"
#include <Preferences.h>
#include <cstring>
#include <cstdio>
#include <atomic>
#include <cstdlib>
namespace LocationLabel {
namespace {
Snapshot store;
std::atomic<uint32_t> active{0};
bool automatic=false;
char connected[33]{};
const char* message="Manual label; no GPS or automatic location measurement.";
uint32_t hash(const void* p,size_t n){uint32_t h=2166136261u;const uint8_t* q=(const uint8_t*)p;while(n--)h=(h^*q++)*16777619u;return h;}
uint32_t keyOf(const char* s){uint32_t h=hash(s,strlen(s))&0xffffffu;return h?h:1;}
bool validText(const char* s){size_t n=0;bool word=false;for(;n<=MAX_TEXT&&s[n];n++){unsigned char c=s[n];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' '||c=='-'||c=='_'||c=='.'))return false;if(c!=' '){if(!word&&c=='-')return false;word=true;}}return n>0&&n<=MAX_TEXT&&word;}
uint32_t checksum(const Snapshot& s){return hash(&s,offsetof(Snapshot,checksum));}
// NVS is only 20 KiB and is shared with settings and credentials. A padded
// 64-entry Snapshot consumed 2296 bytes even for one four-letter label, and
// replacing that blob needs additional free NVS entries. Store only populated
// records; backup snapshots retain their existing, validated layout.
constexpr size_t COMPACT_MAX=10+MAX_LABELS*(4+MAX_TEXT)+MAX_NETWORKS*(4+32);
size_t compactSize(){size_t n=10;for(const auto& e:store.labels)if(e.key)n+=4+strlen(e.text);for(const auto& e:store.networks)if(e.key)n+=4+strlen(e.ssid);return n;}
void put32(uint8_t* p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=uint8_t(v>>(8*i));}
uint32_t get32(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
void encode(uint8_t* b,size_t n){
 put32(b,0x32434f4cu);b[4]=b[5]=0;size_t at=6;
 for(const auto& e:store.labels)if(e.key){b[4]++;size_t len=strlen(e.text);put32(b+at,e.key);b[at+3]=uint8_t(len);at+=4;memcpy(b+at,e.text,len);at+=len;}
 for(const auto& e:store.networks)if(e.key){b[5]++;size_t len=strlen(e.ssid);put32(b+at,e.key);b[at+3]=uint8_t(len);at+=4;memcpy(b+at,e.ssid,len);at+=len;}
 put32(b+n-4,hash(b,n-4));
}
bool decode(const uint8_t* b,size_t n){
 if(n<10||n>COMPACT_MAX||get32(b)!=0x32434f4cu||b[4]>MAX_LABELS||b[5]>MAX_NETWORKS||get32(b+n-4)!=hash(b,n-4))return false;
 store=Snapshot{};size_t at=6;
 for(unsigned kind=0;kind<2;kind++)for(unsigned i=0;i<b[4+kind];i++){
  if(at+4>n-4)return false;uint32_t key=get32(b+at)&0xffffffu;size_t len=b[at+3];at+=4;
  if(!key||!len||len>(kind?32:MAX_TEXT)||len>n-4-at)return false;
  if(kind){store.networks[i].key=key;memcpy(store.networks[i].ssid,b+at,len);}else{store.labels[i].key=key;memcpy(store.labels[i].text,b+at,len);}at+=len;
 }
 store.checksum=checksum(store);return at==n-4&&validate(store);
}
bool persist(){
 store.checksum=checksum(store);const size_t size=compactSize();
 // One strictly bounded allocation avoids a 2 KiB stack frame in loopTask.
 uint8_t* bytes=(uint8_t*)malloc(size);if(!bytes)return false;encode(bytes,size);
 Preferences p;bool ok=false;if(p.begin("dnsp-location",false)){ok=p.putBytes("v2",bytes,size)==size;if(ok&&p.isKey("v1"))p.remove("v1");p.end();}free(bytes);return ok;
}
const Entry* find(uint32_t key){for(const auto& e:store.labels)if(e.key==key&&key)return &e;return nullptr;}
}
const char* text(uint32_t key){if(!key)return "no-label-set";const Entry* e=find(key);return e?e->text:"location-unavailable";}
const char* current(){return text(active.load(std::memory_order_relaxed));}uint32_t currentKey(){return active.load(std::memory_order_relaxed);}
bool recalled(){return automatic;}const char* status(){return message;}
bool validate(const Snapshot& s){
 if(s.magic!=0x31434f4cu||s.checksum!=checksum(s))return false;
 for(unsigned i=0;i<MAX_LABELS;i++){const auto& e=s.labels[i];if(e.key&&(!memchr(e.text,0,sizeof e.text)||!validText(e.text)||keyOf(e.text)!=e.key))return false;for(unsigned j=0;j<i;j++)if(e.key&&e.key==s.labels[j].key)return false;}
 for(unsigned i=0;i<MAX_NETWORKS;i++){const auto& n=s.networks[i];if(!memchr(n.ssid,0,sizeof n.ssid))return false;if(n.key){bool found=false;for(const auto& e:s.labels)if(e.key==n.key)found=true;if(!n.ssid[0]||!found)return false;}for(unsigned j=0;j<i;j++)if(n.ssid[0]&&!strcmp(n.ssid,s.networks[j].ssid))return false;}
 return true;
}
void begin(){
 store=Snapshot{};active=0;automatic=false;connected[0]=0;message="Manual label; no GPS or automatic location measurement.";
 Preferences p;if(!p.begin("dnsp-location",true))return;bool loaded=false;
 const size_t n=p.isKey("v2")?p.getBytesLength("v2"):0;if(n>=10&&n<=COMPACT_MAX){uint8_t* b=(uint8_t*)malloc(n);if(b){loaded=p.getBytes("v2",b,n)==n&&decode(b,n);free(b);}}
 if(!loaded){store=Snapshot{};const bool legacy=p.isKey("v1")&&p.getBytesLength("v1")==sizeof store&&p.getBytes("v1",&store,sizeof store)==sizeof store&&validate(store);if(!legacy)store=Snapshot{};}
 p.end();
}
bool set(const char* label){
 if(!label||!validText(label)||label[0]=='-'){message="Use 1-24 letters, numbers, spaces, dash, dot or underscore.";return false;}
 const uint32_t key=keyOf(label);Entry* slot=nullptr;
 for(auto& e:store.labels)if(e.key==key){if(strcmp(e.text,label)){message="Label ID conflict. Choose a different label.";return false;}slot=&e;break;}
 if(!slot)for(auto& e:store.labels)if(!e.key){slot=&e;break;}
 if(!slot){message="64 historical labels used. Reuse an existing label.";return false;}
 Network* network=nullptr;
 if(connected[0]){for(auto& n:store.networks)if(!strcmp(n.ssid,connected)){network=&n;break;}if(!network)for(auto& n:store.networks)if(!n.ssid[0]){network=&n;break;}if(!network){message="Six network labels used. Clear one first.";return false;}}
 const Entry old=*slot;Network oldNet;if(network)oldNet=*network;
 slot->key=key;snprintf(slot->text,sizeof slot->text,"%s",label);if(network){snprintf(network->ssid,sizeof network->ssid,"%s",connected);network->key=key;}
 if(!persist()){*slot=old;if(network)*network=oldNet;message="Label could not be saved. Try again.";return false;}
 active=key;automatic=false;message=network?"Set manually; saved for this authenticated Wi-Fi network.":"Set manually for this session.";return true;
}
bool clear(){Network* network=nullptr;for(auto& n:store.networks)if(connected[0]&&!strcmp(n.ssid,connected)){network=&n;break;}if(network){const Network old=*network;*network=Network{};if(!persist()){*network=old;message="Could not forget network label. Try again.";return false;}}active=0;automatic=false;message="Label cleared; current network association forgotten.";return true;}
bool rememberNetwork(const char* ssid){if(!active||!ssid||!ssid[0]||strlen(ssid)>32){message="Set a label and authenticate to Wi-Fi first.";return false;}Network* slot=nullptr;for(auto& n:store.networks)if(!strcmp(n.ssid,ssid)){slot=&n;break;}if(!slot)for(auto& n:store.networks)if(!n.ssid[0]){slot=&n;break;}if(!slot){message="Six network labels used. Clear one first.";return false;}const Network old=*slot;snprintf(slot->ssid,sizeof slot->ssid,"%s",ssid);slot->key=currentKey();if(!persist()){*slot=old;message="Network label could not be saved.";return false;}message="Label remembered for the Wi-Fi verified this boot.";return true;}
bool forgetNetwork(const char* ssid){if(!ssid||!ssid[0])return true;for(auto& n:store.networks)if(!strcmp(n.ssid,ssid)){const Network old=n;n=Network{};if(!persist()){n=old;return false;}break;}return true;}
void wifi(const char* ssid){ssid=ssid?ssid:"";if(!strcmp(ssid,connected))return;if(automatic){active=0;automatic=false;}snprintf(connected,sizeof connected,"%s",ssid);if(!connected[0])return;for(const auto& n:store.networks)if(!strcmp(n.ssid,connected)&&n.key){active=n.key;automatic=true;message="Recalled after successful Wi-Fi connection.";return;}}
void capture(Snapshot& out){out=store;out.checksum=checksum(out);}
bool restore(const Snapshot& in){
 if(!validate(in))return false;
 // Preflight capacity and collisions before changing anything. Never evict
 // label text still referenced by the fixed 64-byte black-box records.
 unsigned empty=0,needed=0;for(const auto& e:store.labels)if(!e.key)empty++;
 for(const auto& e:in.labels)if(e.key){const Entry* old=find(e.key);if(old&&strcmp(old->text,e.text))return false;if(!old)needed++;}
 if(needed>empty)return false;
 unsigned netFree=0,netNeeded=0;for(const auto& n:store.networks)if(!n.ssid[0])netFree++;
 for(const auto& n:in.networks)if(n.ssid[0]){bool found=false;for(const auto& old:store.networks)if(!strcmp(old.ssid,n.ssid))found=true;if(!found)netNeeded++;}
 if(netNeeded>netFree)return false;
 Snapshot old=store;
 for(const auto& e:in.labels)if(e.key&&!find(e.key))for(auto& dst:store.labels)if(!dst.key){dst=e;break;}
 for(const auto& n:in.networks)if(n.ssid[0]){Network* dst=nullptr;for(auto& a:store.networks)if(!strcmp(a.ssid,n.ssid)){dst=&a;break;}if(!dst)for(auto& a:store.networks)if(!a.ssid[0]){dst=&a;break;}if(dst)*dst=n;}
 if(!persist()){store=old;return false;}return true;
}
void wipe(){store=Snapshot{};active=0;automatic=false;connected[0]=0;persist();}
}
