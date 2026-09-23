#pragma once
#include <stdint.h>
namespace Breakout {
enum class Phase : uint8_t { READY, PLAYING, PAUSED, WON, LOST };
// Fixed logical 320x200 court. No allocation, radio, storage or display calls.
struct Game {
    static constexpr float Radius=3, PaddleHalf=25, PaddleY=185;
    uint32_t bricks=0xffffffffu;
    uint16_t score=0;
    uint8_t lives=3;
    float x=160,y=180,vx=65,vy=-115,paddle=160;
    Phase phase=Phase::READY, resumePhase=Phase::READY;
    void reset();
    void move(float center);
    void launch();
    void pause();
    void resume();
    void step(); // exactly 8 ms; caller limits catch-up work
};
}
