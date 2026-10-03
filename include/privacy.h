// SquachWatch-CYD — PRIVACY MODE: what the screen shows of other people.
//
// For filming. With it on, nothing drawn identifies a particular device,
// person or network: an address keeps its maker half and loses its device
// half, and a name a device or network gave itself keeps three characters.
// Types, makers, signal strength and counts are untouched -- they say what
// kind of thing is near, not whose.
//
// Screen only. The log, the black box, the wardrive file and the console
// keep the real values; this is a lens, not an eraser.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "settings.h"

namespace Privacy {

inline bool on() { return Settings::privacyMode(); }

// "A4:C1:38:XX:XX:XX" with it on; all six bytes otherwise. 18 bytes.
inline void mac(char* out, size_t n, const uint8_t* m) {
    if (on()) snprintf(out, n, "%02X:%02X:%02X:XX:XX:XX", m[0], m[1], m[2]);
    else      snprintf(out, n, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

// A name as the screen may show it: "Gar***" for "Gary's iPhone". Returns
// `in` itself when the mode is off, when there is no name, or when it is one
// of the firmware's own placeholders ("(hidden)", "Unnamed device").
inline const char* name(const char* in, char* buf, size_t n) {
    if (!on() || !in || !in[0] || in[0] == '(' || n < 8) return in;
    if (strcmp(in, "Unnamed device") == 0 || strcmp(in, "UNKNOWN DEVICE") == 0) return in;
    const size_t len = strlen(in);
    snprintf(buf, n, "%.*s***", (int)(len < 3 ? 1 : 3), in);
    return buf;
}

}
