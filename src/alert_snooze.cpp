#include "alert_snooze.h"
namespace AlertSnooze {
static bool paused=false;static uint32_t began=0;static uint16_t counts[32]{};static uint16_t seen=0;
void start(uint32_t now){began=now;paused=true;seen=0;for(auto& n:counts)n=0;}
void resume(){paused=false;}
uint32_t remaining(uint32_t now){if(!paused)return 0;uint32_t elapsed=now-began;if(elapsed>=DURATION_MS){paused=false;return 0;}return DURATION_MS-elapsed;}
bool active(uint32_t now){return remaining(now)!=0;}
void note(uint8_t type){if(!paused)return;if(seen!=0xffff)seen++;if(type<32&&counts[type]!=0xffff)counts[type]++;}
uint16_t total(){return seen;}
uint8_t topType(){uint8_t best=0;for(uint8_t i=1;i<32;i++)if(counts[i]>counts[best])best=i;return best;}
}
