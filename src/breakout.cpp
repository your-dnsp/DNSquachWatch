#include "breakout.h"
namespace Breakout {
static float clamp(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
void Game::reset(){*this=Game{};}
void Game::move(float center){paddle=clamp(center,PaddleHalf,320-PaddleHalf);if(phase==Phase::READY)x=paddle;}
void Game::launch(){if(phase==Phase::READY){x=paddle;y=PaddleY-Radius-1;vx=65;vy=-115;phase=Phase::PLAYING;}}
void Game::pause(){if(phase==Phase::PLAYING||phase==Phase::READY){resumePhase=phase;phase=Phase::PAUSED;}}
void Game::resume(){if(phase==Phase::PAUSED)phase=resumePhase;}
void Game::step(){
    if(phase!=Phase::PLAYING)return;
    const float ox=x,oy=y;
    x+=vx*0.008f;y+=vy*0.008f;
    if(x<Radius){x=Radius;vx=vx<0?-vx:vx;}
    if(x>320-Radius){x=320-Radius;vx=vx>0?-vx:vx;}
    if(y<Radius){y=Radius;vy=vy<0?-vy:vy;}
    if(vy>0 && oy+Radius<=PaddleY && y+Radius>=PaddleY && x+Radius>=paddle-PaddleHalf && x-Radius<=paddle+PaddleHalf){
        y=PaddleY-Radius;float offset=clamp((x-paddle)/PaddleHalf,-1,1);
        vx=offset*110; if(vx>-20&&vx<20)vx=vx<0?-20:20;
        vy=-115;
    }
    for(unsigned i=0;i<32;++i){
        if(!(bricks&(uint32_t(1)<<i)))continue;
        const float left=8+(i%8)*38,top=20+(i/8)*15,right=left+34,bottom=top+11;
        if(x+Radius<left||x-Radius>right||y+Radius<top||y-Radius>bottom)continue;
        bricks &= ~(uint32_t(1)<<i);score+=10;
        if(oy+Radius<=top){y=top-Radius;vy=vy>0?-vy:vy;}
        else if(oy-Radius>=bottom){y=bottom+Radius;vy=vy<0?-vy:vy;}
        else if(ox<left){x=left-Radius;vx=vx>0?-vx:vx;}
        else {x=right+Radius;vx=vx<0?-vx:vx;}
        if(!bricks)phase=Phase::WON;
        break;
    }
    if(y-Radius>200){
        if(lives)--lives;
        phase=lives?Phase::READY:Phase::LOST;x=paddle;y=PaddleY-Radius-1;
    }
}
}
