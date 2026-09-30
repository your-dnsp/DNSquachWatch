#pragma once
#include <stddef.h>
#include <stdint.h>
struct AdvertField {
    const uint8_t* bytes;
    size_t length;
    AdvertField(const uint8_t* p=nullptr, size_t n=0) : bytes(p), length(n) {}
    const uint8_t* data() const { return bytes; }
    size_t size() const { return length; }
    uint8_t operator[](size_t i) const { return bytes[i]; }
};
inline AdvertField findAdvertField(const uint8_t* payload, size_t size, uint8_t type, unsigned index=0) {
    for (size_t at=0; at<size;) {
        const size_t len=payload[at];
        if (!len) { ++at; continue; }
        if (len>size-at-1) break;
        const uint8_t found=payload[at+1];
        if (found==type || (type==0x09 && found==0x08)) {
            if (!index) return {payload+at+2,len-1};
            --index;
        }
        at+=len+1;
    }
    return {};
}
