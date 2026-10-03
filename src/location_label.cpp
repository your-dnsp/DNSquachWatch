#include "location_label.h"
#include <Preferences.h>
#include <cstring>
#include <cstdio>
#include <atomic>
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
bool persist(){store.checksum=checksum(store);Preferences p;if(!p.begin("dnsp-location",false))return false;bool ok=p.putBytes("v1",&store,sizeof store)==sizeof store;p.end();return ok;}
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
void begin(){store=Snapshot{};active=0;automatic=false;connected[0]=0;Preferences p;Snapshot temp;if(p.begin("dnsp-location",true)){bool ok=p.getBytesLength("v1")==sizeof temp&&p.getBytes("v1",&temp,sizeof temp)==sizeof temp;p.end();if(ok&&validate(temp))store=temp;}}
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
