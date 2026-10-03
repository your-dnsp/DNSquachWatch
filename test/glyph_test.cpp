#include "test_util.h"
#include "glyph_decode.h"
#include "language_data.h"
#include "glyph_expected.h"
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
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
    std::ifstream file("../microSD-content/DNSP Content/v1.5/glyphs.bin",std::ios::binary);
    std::vector<uint8_t> external((std::istreambuf_iterator<char>(file)),{});
    ck("card font equals retained source fixture",external.size()==sizeof glyphBits&&memcmp(external.data(),glyphBits,sizeof glyphBits)==0);
    all=external.size()==sizeof glyphBits;
    if(all)for(size_t i=0;i<sizeof glyphs/sizeof glyphs[0];++i){const auto& g=glyphs[i];for(unsigned jp=0;jp<2;++jp){
        size_t off=jp?g.jpOffset:g.offset;size_t n=std::min(size_t(64),external.size()-off);
        bool ok=GlyphDecode::decode(external.data()+off,n,0,g.width,out);uint32_t hash=2166136261u;
        for(unsigned j=0;j<g.width*2;++j)hash=(hash^out[j])*16777619u;
        all&=ok&&hash==expectedGlyph[i][jp];
    }}
    ck("bounded card windows preserve every normal/Japanese glyph",all);
    ck("bad offset rejected",!GlyphDecode::decode(glyphBits,sizeof glyphBits,sizeof glyphBits,8,out));
    ck("bad width rejected",!GlyphDecode::decode(glyphBits,sizeof glyphBits,0,24,out));
    all=true;
    for(const auto& g:glyphs){
        size_t next=sizeof glyphBits;
        for(const auto& h:glyphs){if(h.offset>g.offset&&h.offset<next)next=h.offset;if(h.jpOffset>g.offset&&h.jpOffset<next)next=h.jpOffset;}
        for(size_t cut=0;cut<next-g.offset;++cut)all &= !GlyphDecode::decode(glyphBits+g.offset,cut,0,g.width,out);
    }
    ck("all truncated glyph prefixes rejected",all);
    return report();
}
