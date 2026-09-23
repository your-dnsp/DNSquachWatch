#include "test_util.h"
#include "breakout.h"
#include <cmath>
using namespace Breakout;
int main(){
    Game g;
    suite("Breakout rules and bounded state");
    ck("starts with 32 bricks and three lives",g.bricks==0xffffffffu&&g.lives==3&&g.phase==Phase::READY);
    g.move(-100);ck("paddle clamps left",g.paddle==25&&g.x==25);
    g.move(999);ck("paddle clamps right",g.paddle==295&&g.x==295);
    g.launch();g.step();ck("launch starts motion",g.phase==Phase::PLAYING&&g.y<181);
    g.pause();float x=g.x,y=g.y;for(int i=0;i<1000;++i)g.step();
    ck("paused state never advances",g.x==x&&g.y==y&&g.phase==Phase::PAUSED);
    g.pause();g.resume();ck("repeated pause preserves resume phase",g.phase==Phase::PLAYING);
    g.x=3;g.vx=-100;g.y=100;g.step();ck("left wall reflects",g.x==3&&g.vx>0);
    g.x=317;g.vx=100;g.step();ck("right wall reflects",g.x==317&&g.vx<0);
    g.y=3;g.vy=-100;g.step();ck("ceiling reflects",g.y==3&&g.vy>0);
    g.move(160);g.x=160;g.y=181.5f;g.vx=0;g.vy=115;g.step();
    ck("descending ball bounces off paddle",g.vy<0&&g.lives==3&&g.vx!=0);
    g.x=20;g.y=16.5f;g.vx=0;g.vy=115;g.step();
    ck("brick removed and scored once",!(g.bricks&1)&&g.score==10&&g.vy<0);
    g.y=16.5f;g.vy=115;g.step();ck("cleared brick cannot score again",g.score==10);
    g.bricks=1;g.y=16.5f;g.vy=115;g.step();ck("last brick wins",g.phase==Phase::WON&&g.score==20);
    g.reset();g.launch();
    for(int i=0;i<3;++i){g.y=205;g.step();if(i<2){ck("miss requires new serve",g.phase==Phase::READY);g.launch();}}
    ck("three misses end round",g.phase==Phase::LOST&&g.lives==0);
    for(int i=0;i<100;++i)g.step();ck("terminal state cannot underflow lives",g.lives==0);
    g.reset();g.pause();g.resume();ck("ready resumes without auto launching",g.phase==Phase::READY);
    suite("Long deterministic play cannot escape bounds or corrupt state");
    bool ok=true;uint32_t seed=17;
    for(int i=0;i<100000;++i){
        seed=seed*1664525u+1013904223u;
        g.move(float(seed%360)-20);
        if(g.phase==Phase::LOST||g.phase==Phase::WON)g.reset();
        if(g.phase==Phase::READY)g.launch();
        if(i%79==0){g.pause();g.resume();}
        g.step();ok &= std::isfinite(g.x)&&std::isfinite(g.y)&&g.x>=3&&g.x<=317&&g.lives<=3&&g.score<=320;
    }
    ck("100000 physics steps stay bounded",ok);
    return report();
}
