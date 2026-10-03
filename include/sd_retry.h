#pragma once
#include <stdint.h>
namespace SdRetry {
// Retry state belongs to the pending queue head. Never accumulates more RAM
// for a slow/full card; a retired failure is counted and retained flash
// history remains the recovery source. Unsigned differences handle rollover.
struct State {
 uint32_t at=0;uint8_t attempts=0;
 bool due(uint32_t now)const{return !attempts||now-at>=500u;}
 bool retire(bool success,bool mounted,uint32_t now){
  at=now;if(!success&&mounted&&++attempts<3)return false;
  attempts=0;return true;
 }
};
}
