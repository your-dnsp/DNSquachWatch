#pragma once
#include <stddef.h>

// Radio names are untrusted bytes, not spreadsheet expressions or CSV syntax.
// Bound both input and output, flatten control characters and delimiters, and
// block a formula even when whitespace precedes its first character.
inline void safeCsvText(char* out, size_t cap, const char* in, size_t inputCap) {
    if (!cap) return;
    size_t n = 0;
    bool first = true;
    while (in && n + 1 < cap && n < inputCap && in[n]) {
        unsigned char c = (unsigned char)in[n];
        if (c < 32 || c > 126 || c == ',' || c == '"') c = '.';
        if (first && c != ' ') {
            if (c == '=' || c == '+' || c == '-' || c == '@') c = '?';
            first = false;
        }
        out[n++] = (char)c;
    }
    out[n] = 0;
}
