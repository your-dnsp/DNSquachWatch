#pragma once
#include <stdint.h>
namespace Remington {
constexpr uint32_t PASS_MS=7000;
struct TripleTap {
 uint32_t first=0,last=0;uint8_t count=0;
 bool tap(uint32_t now){
  if(!count||uint32_t(now-last)>600||uint32_t(now-first)>1200){count=1;first=last=now;return false;}
  last=now;if(++count<3)return false;count=0;return true;
 }
};
}
