#pragma once
#include <stdint.h>
// Manual screen-off is sticky. Alerts borrow it without changing that choice.
// Unsigned elapsed times remain safe across millis() rollover.
namespace PocketScreen {
struct State {
    bool active=false;
    uint32_t pressedAt=0, noteAt=0;
    void exit(){active=false;noteAt=0;}
    void press(uint32_t now,bool dimmed,bool showNote) {
        if(active){exit();return;}
        if(dimmed)return; // wake an automatic timeout, rather than entering pocket mode
        active=true;pressedAt=now;noteAt=showNote?now:0;
    }
    bool dark(uint32_t now,bool alert,uint32_t shownAt,uint32_t duration,bool wake) const {
        if(!active)return false;
        if(noteAt && now-noteAt<4000)return false;
        const bool newer=(int32_t)(shownAt-pressedAt)>0;
        return !(alert && wake && newer && now-shownAt<duration);
    }
};
}
