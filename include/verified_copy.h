#pragma once
#include "dnsp_sha256.h"
namespace Backup {
// Only caller-owned storage callbacks. One 1 KiB operation per tick, no heap.
// Completion requires a second pass over the saved bytes and exact length.
struct Copy {
 enum class Phase:uint8_t {IDLE,WRITE,VERIFY,DONE,FAILED};
 Phase phase=Phase::IDLE;uint32_t size=0,at=0;uint8_t digest[32]{};DnspHash::Sha256 hash;
 void start(uint32_t n){size=n;at=0;phase=n?Phase::WRITE:Phase::FAILED;DnspHash::shaInit(hash);}
 template<class Source,class Destination> void tick(Source& source,Destination& dest){
  if(phase!=Phase::WRITE&&phase!=Phase::VERIFY)return;
  uint8_t data[1024];size_t n=size-at;if(n>sizeof data)n=sizeof data;
  bool ok=phase==Phase::WRITE?(source.read(at,data,n)&&dest.write(data,n)):dest.read(data,n);
  if(!ok){phase=Phase::FAILED;return;}DnspHash::shaUpdate(hash,data,n);at+=n;
  if(at==size){uint8_t d[32];DnspHash::shaFinal(hash,d);
   if(phase==Phase::WRITE){memcpy(digest,d,32);if(!dest.rewind(size)){phase=Phase::FAILED;return;}phase=Phase::VERIFY;at=0;DnspHash::shaInit(hash);}
   else phase=memcmp(d,digest,32)?Phase::FAILED:Phase::DONE;
  }
 }
};
}
