#pragma once
#include <stdint.h>
namespace CardContent {
constexpr uint8_t guideCount=74;
struct Guide { char title[48]; char body[192]; };
bool verifyRequired(); // stream hashes of required v1.5 card assets, one handle
void reset();
void guide(uint8_t page,bool mounted,Guide& view);
}
