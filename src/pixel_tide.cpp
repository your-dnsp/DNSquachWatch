// Original fixed-memory ripple toy; no imported simulation or SD assets.
#include "pixel_tide.h"
#include "ui_scratch.h"
#include <string.h>
namespace PixelTide {
static unsigned current=0,ticks=0;
void reset(){UiScratch::storage=UiScratch::Storage{};current=ticks=0;}
void touch(int x,int y,int width,int height){
    if(width<=0||height<=0||x<0||y<0||x>=width||y>=height)return;
    unsigned a=(unsigned)x*W/width,b=(unsigned)y*H/height;
    if(a && a<W-1 && b && b<H-1)UiScratch::storage.tide[current][b][a]=1000;
}
void step(){
    auto& a=UiScratch::storage.tide[current];auto& b=UiScratch::storage.tide[current^1];
    if(++ticks%45==0)a[3+(ticks/45*7)%(H-6)][3+(ticks/45*11)%(W-6)]=650;
    for(unsigned y=1;y<H-1;y++)for(unsigned x=1;x<W-1;x++){
        int v=(a[y-1][x]+a[y+1][x]+a[y][x-1]+a[y][x+1])/2-b[y][x];
        v-=v/20; if(v>1600)v=1600;if(v< -1600)v=-1600;b[y][x]=(int16_t)v;
    }
    current^=1;
}
uint16_t color(unsigned x,unsigned y){
    if(x>=W||y>=H)return 0;
    int v=UiScratch::storage.tide[current][y][x];if(v<0)v=-v;
    unsigned glow=(unsigned)v/32;if(glow>31)glow=31;
    unsigned r=2+glow/2,g=9+glow,b=12+glow/2;
    return (uint16_t)((r<<11)|(g<<5)|b);
}
}
