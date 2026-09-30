#pragma once
#include "detection.h"
#include "ui_settings.h"
#include <TFT_eSPI.h>
namespace CareUI {
enum class Page:uint8_t {BACKUP,DEMO,STATUS,GIFT,REPORT,HEALTH,WELCOME,SD_RECOVERY,READABLE_LOGS};
void open(Page=Page::BACKUP);
Page page();
void runPending(bool visible, uint32_t now, DetectionEngine&);
bool working();
void drawBackupProgress(TFT_eSPI&, unsigned percent, const char* phase, uint32_t now);
bool needsDraw(uint32_t now,int width,int height);
void draw(TFT_eSPI&,uint32_t,DetectionEngine&);
SettingsRow tap(int x,int y,int w,int h,uint32_t now,DetectionEngine&); // BACK exits
}
