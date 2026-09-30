#pragma once
#include <stdint.h>
namespace PixelTide {
constexpr unsigned W=40,H=26;
void reset();
void touch(int x,int y,int width,int height);
void step();
uint16_t color(unsigned x,unsigned y);
}
