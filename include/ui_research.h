#pragma once
#include <TFT_eSPI.h>
namespace ResearchUI {
void open();
void draw(TFT_eSPI& t,uint32_t now);
// Returns true for Back. All actions occur once, outside the renderer.
bool tap(int x,int y,int w,int h,uint32_t now,bool cardReady,uint32_t sessionId);
}
