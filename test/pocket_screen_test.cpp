#include "pocket_screen.h"
#include "test_util.h"
int main(){
 suite("Pocket Mode preserves manual screen-off");PocketScreen::State p;
 p.press(100,false,false);ck("manual press darkens",p.dark(101,false,0,15000,true));
 ck("an existing alert does not undo press",p.dark(101,true,90,15000,true));
 ck("later alert borrows screen",!p.dark(300,true,200,15000,true));
 ck("configured duration remains visible",!p.dark(15199,true,200,15000,true));
 ck("screen darkens at end",p.dark(15200,true,200,15000,true));
 ck("disabled wake stays dark",p.dark(300,true,200,15000,false));
 ck("closing card restores darkness",p.dark(400,false,200,15000,true));
 p.press(500,true,false);ck("second button press exits",!p.active);
 p.press(600,true,false);ck("automatic dim wake does not enter pocket",!p.active);
 p.press(700,false,true);ck("first-time note stays visible",!p.dark(701,false,0,15000,true));
 ck("note ends after four seconds",p.dark(4700,false,0,15000,true));
 p.exit();p.press(0xfffffff0U,false,false);
 ck("later alert across millis rollover borrows screen",!p.dark(20,true,10,15000,true));
 ck("rollover duration expires",p.dark(15010,true,10,15000,true));
 return report();
}
