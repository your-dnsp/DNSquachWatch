#pragma once
#include "detection.h"
#include <TFT_eSPI.h>
namespace FieldUI {
#if !defined(ARDUINO_ARCH_ESP32)
uint32_t drawCount();
#endif
bool needsDraw(uint32_t now, int width, int height);
void open();
void openHelp();
void openLanguage();
void openAccessibility();
void openRules();
void draw(TFT_eSPI &t, uint32_t now, const DetectionEngine &engine);
bool tap(int x, int y, int w, int h, uint32_t now, const DetectionEngine &engine);
uint8_t currentPage();
} // namespace FieldUI
