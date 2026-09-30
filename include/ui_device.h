#pragma once
#include <TFT_eSPI.h>
namespace DeviceUI {
enum Page { HELP, GENERAL, POWER, DATA, REPORTS, READER };
void open(Page page,bool sdReady,int w,int h);
void draw(TFT_eSPI& t);
bool touch(int x,int y,int w,int h); // true: back to System
}
