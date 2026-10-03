#include "simulation.h"
#include <cstring>
#include <cstdio>
namespace Simulation {
// Share the byte check across UI/export paths instead of duplicating it in
// every caller in this nearly-full image. Never called in the radio callback.
__attribute__((noinline)) bool marked(const uint8_t* mac){return mac&&mac[3]==0&&mac[4]==0&&mac[5]==0;}
const char* roleName(AddressRole role){switch(role){case AddressRole::TRANSMITTER:return "claimed transmitter";case AddressRole::BSSID:return "BSSID";case AddressRole::BLE_ADVERTISER:return "BLE advertiser";default:return "matched address";}}
const char* note(){return "SIMULATED: DNSP test-address convention; decoded MAC suffix is all zero. Real hardware may use it too; not reserved or authenticated.";}
bool finishJson(char* out,size_t cap,size_t at,const uint8_t* mac,AddressRole role){
 if(!out||at>=cap)return false;bool sim=marked(mac);int n=snprintf(out+at,cap-at,",\"simulated\":%s,\"simulation_subtag\":\"%s\",\"address_provenance\":\"%s\"}\n",sim?"true":"false",sim?"SIMULATED":"",roleName(role));return n>=0&&size_t(n)<cap-at;
}
void subtags(char* out,size_t cap,const char* user,const uint8_t* mac){
 if(!out||!cap)return;snprintf(out,cap,"%s",user?user:"");if(!marked(mac))return;
 // Match a complete subtag token, not SIMULATED inside another user word.
 const char* p=out;while((p=strstr(p,"SIMULATED"))){bool left=p==out||p[-1]=='/'||p[-1]==' '||p[-1]==',';char c=p[9];if(left&&(!c||c=='/'||c==' '||c==','))return;p+=9;}
 size_t n=strlen(out);snprintf(out+n,cap-n,"%sSIMULATED",n?" / ":"");
}
}
