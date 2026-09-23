#pragma once
#include <stddef.h>
#include <stdint.h>
namespace GlyphDecode {
// Always bounded, including malformed offsets/widths; no heap allocation.
inline bool decode(const uint8_t* blob,size_t size,size_t offset,uint8_t width,uint8_t out[32]) {
    if(!blob||!out||(width!=8&&width!=16)||offset>size||size-offset<2) return false;
    unsigned stride=width/8;
    uint16_t mask=uint16_t(blob[offset]) | (uint16_t(blob[offset+1])<<8);
    offset+=2; uint8_t row[2]={0,0};
    for(unsigned r=0;r<16;++r) {
        if(mask & (uint16_t(1)<<r)) {
            if(size-offset<stride) return false;
            for(unsigned c=0;c<stride;++c) row[c]=blob[offset++];
        }
        for(unsigned c=0;c<stride;++c) out[r*stride+c]=row[c];
    }
    return true;
}
}
