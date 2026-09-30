#pragma once
#include <stdint.h>
namespace AlertSnooze {
constexpr uint32_t DURATION_MS=10UL*60UL*1000UL;
void start(uint32_t now);
void resume();
uint32_t remaining(uint32_t now);
bool active(uint32_t now);
void note(uint8_t type);
uint16_t total();
uint8_t topType();
}
