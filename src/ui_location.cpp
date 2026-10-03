#include "ui_location.h"
#include "location_label.h"
#include "theme.h"
#include "ota_wifi.h"
#include <cstdio>
#include <cstring>
namespace LocationUI {
void draw(TFT_eSPI& t){
 int w=t.width(),h=t.height();t.fillRect(0,0,w,h,Theme::BG);t.setTextFont(1);t.setTextSize(2);t.setTextColor(Theme::CYAN);t.setCursor(8,8);t.print("SET LOCATION");
 t.setTextSize(1);t.setTextColor(Theme::WHITE);t.setCursor(8,32);t.print(LocationLabel::current());t.setCursor(8,45);t.print(!LocationLabel::currentKey()?"No label: findings use no-label-set":LocationLabel::recalled()?"Recalled from authenticated Wi-Fi":"Manual label | 24 characters maximum");
 const char* labels[]={"EDIT LABEL","CLEAR LABEL","Home","Work","Driving","Con"};int bw=(w-24)/2;
 for(int i=0;i<6;i++)Theme::drawWin95Button(t,8+(i%2)*(bw+8),62+(i/2)*32,bw,26,labels[i],false);
 Theme::drawWin95Button(t,8,158,bw,24,"SAVE FOR WI-FI",false);
 Theme::drawWin95Button(t,16+bw,158,bw,24,"FORGET WI-FI",false);
 char network[64];snprintf(network,sizeof network,"%s: %s",OtaWifi::authenticatedNetwork()[0]?"Connected":"Verified this boot",OtaWifi::lastAuthenticatedNetwork()[0]?OtaWifi::lastAuthenticatedNetwork():"none");
 while(t.textWidth(network)>w-16 && strlen(network)>3)network[strlen(network)-1]=0;
 t.setCursor(8,184);t.print(network);
 char saved[64];snprintf(saved,sizeof saved,"Saved for: %s",LocationLabel::associatedNetwork());
 while(t.textWidth(saved)>w-16 && strlen(saved)>3)saved[strlen(saved)-1]=0;
 t.setCursor(8,196);t.print(LocationLabel::associatedNetwork()[0]?saved:"Session only; SAVE FOR WI-FI to remember");
 Theme::drawWin95Button(t,8,h-30,w-16,24,"BACK",false);
}
int hit(TFT_eSPI& t,int x,int y){int w=t.width(),h=t.height(),bw=(w-24)/2;if(y>=h-30&&y<h-6)return 7;if(y>=158&&y<182)return x<16+bw?8:9;for(int i=0;i<6;i++){int bx=8+(i%2)*(bw+8),by=62+(i/2)*32;if(x>=bx&&x<bx+bw&&y>=by&&y<by+26)return i+1;}return 0;}
}
