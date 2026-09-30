#include "ui_rule_alert.h"
#include "theme.h"
#include <cstdio>
void uiRuleAlertDraw(TFT_eSPI& t,const SketchyRule::Incident& in){
    const int w=t.width(),h=t.height();t.fillScreen(Theme::BG);Theme::drawTitleBar(t,"RULE ALERT");
    t.setTextFont(1);t.setTextSize(2);t.setTextColor(Theme::AMBER,Theme::BG);const char* title="SKETCHY ENVIRONMENT";t.setCursor((w-t.textWidth(title))/2,24);t.print(title);
    char lead[80];snprintf(lead,sizeof lead,"%s + DEAUTH within %lus",detectionTypeName(in.alpr.type),(unsigned long)in.gapSeconds);
    t.setTextSize(1);t.setTextColor(Theme::CYAN,Theme::BG);t.setCursor((w-t.textWidth(lead))/2,50);t.print(lead);
    char lines[10][48];const char* body="An ALPR clue and a deauthentication burst were observed in the same area near the same time. Use caution. This does not prove the events are related.";
    uint8_t n=Theme::wrapText(t,body,w-24,lines,10);t.setTextColor(Theme::WHITE,Theme::BG);for(uint8_t i=0;i<n;i++){t.setCursor(12,70+i*11);t.print(lines[i]);}
    t.setTextColor(in.sdExported?Theme::GREEN:Theme::AMBER,Theme::BG);t.setCursor(12,h-75);t.print(in.sdExported?"Saved in Rules + microSD":"Saved in Rules; microSD export pending");
    Theme::drawButton(t,12,h-48,w-24,36,"GOT IT",false);
}
