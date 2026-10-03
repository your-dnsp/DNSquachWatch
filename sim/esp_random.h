// SquachWatch-Sim -- esp_fill_random shim.
//
// IDF 5 moved esp_random()/esp_fill_random() out of esp_system.h into their own
// header, so meshcrypto.cpp includes it directly. On a host neither exists.
//
// This is NOT a secure generator and must never be one by accident: it exists
// so meshcrypto.cpp links in the emulator and in test/meshcrypto_test, where
// nothing is secret and a repeatable stream is actively wanted. The device
// always gets the real hardware RNG, because this file is only ever on the
// include path for host builds (see the -I$(SIM) in sim/Makefile and
// test/Makefile).
#pragma once
#include "esp_system.h"
#include <stddef.h>
#include <stdint.h>

inline void esp_fill_random(void* buf, size_t len) {
    uint8_t* p = (uint8_t*)buf;
    for (size_t i = 0; i < len; i++) p[i] = (uint8_t)(esp_random() & 0xFF);
}
