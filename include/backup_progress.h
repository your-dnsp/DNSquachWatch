#pragma once
#include <stdint.h>
namespace Backup {
// Elapsed wall time is not failure. Only a lack of observable progress expires.
struct ProgressWatch {
 uint32_t began=0,last=0,token=0;
 void start(uint32_t now,uint32_t value=0){began=last=now;token=value;}
 void observe(uint32_t now,uint32_t value){if(value!=token){last=now;token=value;}}
 bool stalled(uint32_t now)const{return uint32_t(now-last)>120000;}
 uint32_t seconds(uint32_t now)const{return uint32_t(now-began)/1000;}
};
}
