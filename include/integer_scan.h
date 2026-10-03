#pragma once
#include <stdio.h>
// The three users of this alias only parse integer/string fields. Newlib's
// integer-only scanner has the same conversion semantics for those formats
// and avoids linking the separate floating-point scanner on the ESP32.
// Host tests use the standard equivalent; no floating-point parsing changes.
#if defined(ARDUINO_ARCH_ESP32)
#define DNSP_INTEGER_SCAN siscanf
#else
#define DNSP_INTEGER_SCAN sscanf
#endif
