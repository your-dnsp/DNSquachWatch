#include "remington.h"
#include "remmy_gesture.h"
#include "theme.h"
#include "draw_band.h"
#include <Arduino.h>
namespace Remington {
namespace {
#include "remmy_photo_data.inc"
#include "remmy_head_data.inc"
bool tapped=false;uint32_t tapAt=0;
uint8_t pixel(const uint8_t* data,unsigned w,unsigned x,unsigned y){unsigned n=y*w+x;uint8_t v=data[n/2];return n&1?v&15:v>>4;}
void paint(TFT_eSPI& t,const uint8_t* data,const uint16_t* palette,unsigned sw,unsigned sh,int x,int y,int dw,int dh,bool alpha){
 unsigned lastRow=~0u;uint8_t row[164];
 for(int dy=DrawBand::top(y);dy<DrawBand::bot(y+dh);dy++){
  const unsigned sy=unsigned(dy-y)*sh/dh;
  if(!data&&sy!=lastRow){unsigned n=0;for(unsigned i=REMMY_PHOTO_ROWS[sy];i<REMMY_PHOTO_ROWS[sy+1];++i){uint8_t v=REMMY_PHOTO_RLE[i];unsigned count=(v>>4)+1;while(count--&&n<sizeof row)row[n++]=v&15;}lastRow=sy;}
  auto color=[&](unsigned px){return data?pixel(data,sw,px,sy):row[px];};int run=0;
  while(run<dw){unsigned sx=unsigned(run)*sw/dw;const uint8_t v=color(sx);int end=run+1;while(end<dw&&color(unsigned(end)*sw/dw)==v)end++;
   if(!alpha||v)t.drawFastHLine(x+run,dy,end-run,palette[v]);run=end;
  }
 }
}
uint32_t nextAt=0,startAt=0,lastAt=0,sequence=0;bool started=false,live=false;
int pathTop=0,pathBottom=0;TripleTap triple;
uint32_t interval(){sequence=sequence*1664525u+1013904223u;return 15000u+sequence%15001u;}
}
bool backgroundTap(uint32_t now){if(!triple.tap(now))return false;started=true;live=false;nextAt=now;lastAt=now-1;return true;}
void open(){tapped=false;tapAt=0;}
bool tap(uint32_t now){if(tapped&&uint32_t(now-tapAt)<450){tapped=false;return true;}tapped=true;tapAt=now;return false;}
void photo(TFT_eSPI& t){int h=t.height()-16,w=h*REMMY_PHOTO_W/REMMY_PHOTO_H;if(w>t.width()){w=t.width();h=w*REMMY_PHOTO_H/REMMY_PHOTO_W;}t.fillRect(0,0,t.width(),t.height(),Theme::BLACK);paint(t,nullptr,REMMY_PHOTO_PAL,REMMY_PHOTO_W,REMMY_PHOTO_H,(t.width()-w)/2,0,w,h,false);t.setTextFont(1);t.setTextSize(1);t.setTextColor(Theme::WHITE,Theme::BLACK);t.setCursor(6,t.height()-12);t.print("Remington | Double-tap to return");}
void shootingStar(TFT_eSPI& t,uint32_t now,int top,int bottom){
 if(bottom-top<42)return;
 // Advance once per timestamp, never once per band. All movement is elapsed
 // time, not frame count; unsigned subtraction also survives millis rollover.
 if(!started){started=true;sequence=now^0x52454d59u;nextAt=now+interval();lastAt=now-1;}
 if(now!=lastAt){lastAt=now;
  if(!live&&int32_t(now-nextAt)>=0){live=true;startAt=now;sequence=sequence*1664525u+1013904223u;pathTop=top+4+int(sequence%unsigned((bottom-top-42)/2+1));pathBottom=bottom-40;}
  if(live&&uint32_t(now-startAt)>=PASS_MS){live=false;nextAt=now+interval();}
 }
 if(!live)return;
 const uint32_t age=now-startAt;const int w=t.width();int x=w+36-int(uint64_t(age)*(w+140)/PASS_MS),y=pathTop+int(uint64_t(age)*(pathBottom-pathTop)/PASS_MS);
 const uint16_t colors[]={Theme::CYAN,Theme::PINK,Theme::VAPOR_YELLOW};
 for(int k=0;k<3;k++)t.drawLine(x+25,y+20+k*3,x+80+k*8,y-8+k*4,colors[k]);
 for(int i=0;i<6;i++){int sx=x+38+i*11,sy=y+11-i*4+int((age/110+i)%5);uint16_t c=colors[i%3];t.drawFastHLine(sx-2,sy,5,c);t.drawFastVLine(sx,sy-2,5,c);}
 paint(t,REMMY_HEAD_DATA,REMMY_HEAD_PAL,REMMY_HEAD_W,REMMY_HEAD_H,x,y,32,38,true);
}
}
