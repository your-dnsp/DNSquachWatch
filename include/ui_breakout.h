#pragma once
#include "breakout.h"
#include <TFT_eSPI.h>
namespace BreakoutUI {
void open(uint32_t now);
void suspend(uint32_t now);
bool input(int x,int y,int w,int h,bool down,bool justDown,uint32_t now); // true = Back
bool tick(uint32_t now);
void draw(TFT_eSPI& t);
const Breakout::Game& game();
}
