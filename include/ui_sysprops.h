// SquachWatch-CYD — SYSTEM PROPERTIES: the window that says an update is out.
//
// It stands in front of the main screen on the first frame after the intro,
// whenever the boot check or a squad member turned up something newer. It
// exists because the old announcement was a line Squachy spoke, and two
// boards in the room talking to each other painted over it before it could
// be read. A window waits instead: nothing else draws until it is answered.
//
// Three tabs, each backed by something the board actually knows -- a tab
// with nothing behind it would be decoration:
//   UPDATE  what is running, what is out, where it was heard, and the button
//   BOARD   the build, both slots, uptime and heap -- DIAGNOSTICS in brief
//   CREDITS DNSP authorship, upstream attribution and original firmware link
#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>

class DetectionEngine;

enum class SysPropsHit : uint8_t {
    NONE,       // a tab, the checkbox, or nothing
    UPDATE_NOW, // download and install it, now
    CLOSE       // LATER, the close box, or a tap outside the window
};

void        uiSysPropsShowBoard();
void        uiSysPropsShowCredits();
void        uiSysPropsInit(TFT_eSPI& t);
// `advance` is false on the second of the 3.5"'s two band passes -- the
// same frame drawn again -- so anything that steps by the call rather
// than by the clock must sit still for it. Other boards draw once.
void        uiSysPropsTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, bool advance = true);
// Where a cursor should stop on the window as it is drawn now: the buttons
// first, then the tabs. For the boards with no touch panel.
uint8_t uiSysPropsStops(TFT_eSPI& t, int16_t* xs, int16_t* ys, uint8_t cap);
SysPropsHit uiSysPropsTouch(TFT_eSPI& t, int x, int y);


void uiSysPropsScroll(int rows);
