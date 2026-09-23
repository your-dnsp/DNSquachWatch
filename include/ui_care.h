#pragma once
#include "detection.h"
#include "ui_settings.h"
#include <TFT_eSPI.h>
namespace CareUI {
enum class Page:uint8_t {HOME,MORE,BACKUP,DEMO,STATUS,FAVORITES,GIFT,REPORT,HEALTH,WELCOME};
void open(Page=Page::HOME);
Page page();
bool needsDraw(uint32_t now,int width,int height);
void draw(TFT_eSPI&,uint32_t,DetectionEngine&);
SettingsRow tap(int x,int y,int w,int h,uint32_t now,DetectionEngine&); // BACK exits
}
