#include <cstring>
#include "ui_breakout.h"
#include "theme.h"
#include <stdio.h>
namespace BreakoutUI {
static Breakout::Game g;
static uint32_t last=0,drawAt=0,opened=0;
static unsigned remainder=0;
static bool dirty=true,initialized=false;
const Breakout::Game& game(){return g;}
void open(uint32_t now){if(!initialized){g.reset();initialized=true;}last=now;opened=now;remainder=0;dirty=true;}
void suspend(uint32_t now){g.pause();last=now;remainder=0;dirty=true;}
bool input(int x,int y,int w,int h,bool down,bool justDown,uint32_t now){
    if(now-opened<300||!down||w<1||h<100)return false;
    if(y>=h-44){
        if(!justDown)return false;
        if(x<w/3){suspend(now);return true;}
        if(x>=2*w/3)g.reset();
        else if(g.phase==Breakout::Phase::PAUSED)g.resume();else g.pause();
        last=now;remainder=0;dirty=true;return false;
    }
    if(y<28)return false;
    if(g.phase!=Breakout::Phase::PAUSED)g.move(float(x)*320/w);
    if(justDown){
        if(g.phase==Breakout::Phase::PAUSED)g.resume();
        else g.launch();
        last=now;remainder=0;
    }
    if(justDown || g.phase==Breakout::Phase::READY || g.phase==Breakout::Phase::PLAYING)dirty=true;return false;
}
bool tick(uint32_t now){
    uint32_t elapsed=now-last;last=now;
    if(g.phase==Breakout::Phase::PLAYING){
        // Slow a frame after a radio/storage stall rather than losing a life
        // while unseen or spending an unbounded time catching up.
        remainder+=elapsed>40?40:elapsed;
        while(remainder>=8){g.step();remainder-=8;}
        if(now-drawAt>=33)dirty=true;
    }else remainder=0;
    bool result=dirty && uint32_t(now-drawAt)>=33;if(result){dirty=false;drawAt=now;}return result;
}
static void button(TFT_eSPI& t,int x,int y,int w,const char* text){
    t.drawRect(x+2,y+2,w-4,40,Theme::CYAN);
    t.setTextColor(Theme::WHITE,Theme::BG);t.setTextSize(1);
    t.setCursor(x+w/2-int(strlen(text))*3,y+18);t.print(text);
}
void draw(TFT_eSPI& t){
    const int w=t.width(),h=t.height(),courtH=h-80;
    auto px=[&](float x){return int(x*w/320);};
    auto py=[&](float y){return 28+int(y*courtH/200);};
    t.fillRect(0,0,w,h,Theme::BG);t.setTextSize(1);t.setTextColor(Theme::WHITE,Theme::BG);
    char info[64];snprintf(info,sizeof info,"SQUACH SNACKS   %u   HEARTS %u",unsigned(g.score),unsigned(g.lives));
    t.setCursor(6,10);t.print(info);
    const uint16_t colors[4]={Theme::CYAN,Theme::GREEN,Theme::AMBER,Theme::WHITE};
    for(unsigned i=0;i<32;++i)if(g.bricks&(uint32_t(1)<<i)){
        int left=px(8+(i%8)*38),top=py(20+(i/8)*15);
        int bw=px(34),bh=py(11)-py(0);
        // Tiny smiling forest snacks. Same collision boxes as classic Breakout.
        t.fillRoundRect(left,top,bw,bh,2,colors[i/8]);
        t.drawPixel(left+bw/3,top+bh/3,Theme::BLACK);
        t.drawPixel(left+2*bw/3,top+bh/3,Theme::BLACK);
        if(bh>5)t.drawFastHLine(left+bw/2-1,top+bh-2,3,Theme::BLACK);
    }
    t.fillRoundRect(px(g.paddle-25),py(185),px(50),6,2,Theme::FUR_LIGHT);
    t.drawFastHLine(px(g.paddle-23),py(185)+1,px(46),Theme::SKIN_TAN);
    t.fillCircle(px(g.x),py(g.y),3,Theme::AMBER);
    t.drawPixel(px(g.x),py(g.y)-3,Theme::GREEN);
    const char* msg=g.phase==Breakout::Phase::READY?"TAP TO LAUNCH":g.phase==Breakout::Phase::PAUSED?"PAUSED - TAP TO RESUME":g.phase==Breakout::Phase::WON?"PICNIC COMPLETE!":g.phase==Breakout::Phase::LOST?"SNACK BREAK - TAP NEW":nullptr;
    if(msg){t.setCursor((w-int(strlen(msg))*6)/2,py(112));t.print(msg);}
    if(g.phase==Breakout::Phase::READY){t.setCursor((w-27*6)/2,py(133));t.print("Drag to steer. Alerts pause.");}
    button(t,0,h-44,w/3,"BACK");button(t,w/3,h-44,w/3,g.phase==Breakout::Phase::PAUSED?"RESUME":"PAUSE");button(t,2*w/3,h-44,w-2*w/3,"NEW");
}
}
