#include "ui_scratch.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main(){UiScratch::storage.reader=UiScratch::Reader{};strcpy(UiScratch::storage.reader.text,"private reader data");PixelTide::reset();auto baseline=PixelTide::color(20,13);assert(!memcmp(UiScratch::storage.tide,UiScratch::Storage{}.tide,sizeof UiScratch::storage.tide));PixelTide::touch(160,120,320,240);assert(PixelTide::color(20,13)!=baseline);PixelTide::step();UiScratch::storage.reader=UiScratch::Reader{};assert(!UiScratch::storage.reader.text[0]&&!UiScratch::storage.reader.names[0][0]);PixelTide::reset();assert(PixelTide::color(20,13)==baseline);static_assert(sizeof(UiScratch::Storage)==4160,"Fixed scratch size");puts("Reader/decoy ownership transitions clear data without heap PASS");}
