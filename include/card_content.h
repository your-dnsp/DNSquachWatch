#pragma once
#include <stdint.h>
namespace CardContent {
constexpr uint8_t guideCount=74;
struct Guide { char title[48]; char body[192]; };
void reset();
void guide(uint8_t page,bool mounted,Guide& view);
}
