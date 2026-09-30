#pragma once
#include "duress_core.h"
namespace DuressDevice {
// available() validates the installed partition table, not just compiled settings.
bool available();
Duress::Boot boot();
bool arm();
void begin(uint32_t now);
void tick(uint32_t now);
bool done();
uint32_t errors();
}
