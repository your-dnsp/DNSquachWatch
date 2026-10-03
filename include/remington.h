#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>
namespace Remington {
bool backgroundTap(uint32_t now);
void open();bool tap(uint32_t now);
void photo(TFT_eSPI& t);
void shootingStar(TFT_eSPI& t,uint32_t now,int top,int bottom);
}
