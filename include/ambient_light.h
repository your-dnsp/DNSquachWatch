#pragma once
#include <stdint.h>
// CYD LDR: lower ADC reading means more light. Reference, not lux.
namespace AmbientLight {
constexpr uint16_t SIMULATED_READING = 200;
inline uint8_t target(uint16_t raw, uint8_t ceiling) {
    // 200 is our medium-low reference. Protect legibility at either extreme.
    const uint16_t level = raw<=200 ? 255-(uint32_t(raw)*111/200) :
        raw>=1600 ? 48 : 144-(uint32_t(raw-200)*96/1400);
    const uint16_t scaled=uint32_t(level)*ceiling/255;
    return scaled<32 ? 32 : uint8_t(scaled);
}
inline uint8_t approach(uint8_t current,uint8_t target) {
    if(current<target) return target-current>4 ? current+4 : target;
    return current-target>4 ? current-4 : target;
}
}
