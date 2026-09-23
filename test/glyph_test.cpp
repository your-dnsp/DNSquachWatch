#include "test_util.h"
#include "glyph_decode.h"
#include "language_data.h"
#include "glyph_expected.h"
#include <cstring>
#include <cstdio>
int main() {
    suite("Every bundled font bitmap remains identical");
    bool all=true;uint8_t out[32];
    for(size_t i=0;i<sizeof glyphs/sizeof glyphs[0];++i){
        const auto& g=glyphs[i];
        for(unsigned jp=0;jp<2;++jp){
            bool ok=GlyphDecode::decode(glyphBits,sizeof glyphBits,jp?g.jpOffset:g.offset,g.width,out);
            uint32_t hash=2166136261u;
            for(unsigned j=0;j<g.width*2;++j)hash=(hash^out[j])*16777619u;
            all &= ok&&hash==expectedGlyph[i][jp];
        }
    }
    ck("normal and Japanese glyphs match original source",all);
    ck("bad offset rejected",!GlyphDecode::decode(glyphBits,sizeof glyphBits,sizeof glyphBits,8,out));
    ck("bad width rejected",!GlyphDecode::decode(glyphBits,sizeof glyphBits,0,24,out));
    uint8_t shortData[3]={255,255,1};
    ck("truncated rows rejected",!GlyphDecode::decode(shortData,3,0,16,out));
    // Every shorter prefix of an encoded glyph must be rejected.
    all=true;
    for(const auto& g:glyphs){
        uint16_t mask=glyphBits[g.offset]|(uint16_t(glyphBits[g.offset+1])<<8);
        size_t n=2;for(unsigned r=0;r<16;++r)if(mask&(1u<<r))n+=g.width/8;
        for(size_t cut=0;cut<n;++cut)all &= !GlyphDecode::decode(glyphBits+g.offset,cut,0,g.width,out);
    }
    ck("all truncated glyph prefixes rejected",all);
    return report();
}
