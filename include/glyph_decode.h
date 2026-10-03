#pragma once
#include <stddef.h>
#include <stdint.h>
#include "glyph_huffman.h"
namespace GlyphDecode {
// Canonical Huffman over the existing row-change encoding. Decode only the
// requested glyph into 32 bytes; no heap, full-font cache or extra framebuffer.
struct Reader {
    const uint8_t* data;size_t size,pos;uint8_t bit=0;
    Reader(const uint8_t* d,size_t s,size_t p):data(d),size(s),pos(p){}
    bool byte(uint8_t& out){
        uint16_t code=0,first=0,index=0;
        for(unsigned len=1;len<16;++len){
            if(pos>=size)return false;
            code=uint16_t((code<<1)|((data[pos]>>(7-bit))&1));if(++bit==8){bit=0;++pos;}
            const uint16_t n=GLYPH_COUNTS[len];
            if(code>=first&&code-first<n){out=GLYPH_SYMBOLS[index+code-first];return true;}
            index+=n;first=uint16_t((first+n)<<1);
        }
        return false;
    }
};
inline bool decode(const uint8_t* blob,size_t size,size_t offset,uint8_t width,uint8_t out[32]) {
    if(!blob||!out||(width!=8&&width!=16)||offset>=size)return false;
    Reader rd{blob,size,offset};uint8_t lo,hi;if(!rd.byte(lo)||!rd.byte(hi))return false;
    const uint16_t mask=uint16_t(lo)|(uint16_t(hi)<<8);const unsigned stride=width/8;uint8_t row[2]={0,0};
    for(unsigned r=0;r<16;++r){
        if(mask&(uint16_t(1)<<r))for(unsigned c=0;c<stride;++c)if(!rd.byte(row[c]))return false;
        for(unsigned c=0;c<stride;++c)out[r*stride+c]=row[c];
    }
    return true;
}
}
