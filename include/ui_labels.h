#pragma once
#include "user_labels.h"
#include <TFT_eSPI.h>

namespace LabelUI {
enum class Outcome : uint8_t { NONE, SAVED, CANCELLED };
void open(const UserLabels::Target& target);
bool active();
void close();
void draw(TFT_eSPI& t);
Outcome tap(int x,int y,int w,int h,uint32_t now);
}
