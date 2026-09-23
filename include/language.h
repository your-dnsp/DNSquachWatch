#pragma once
#include <TFT_eSPI.h>
#include "state.h"
#include <stddef.h>
#include <stdint.h>
namespace Lang {
const char *detectionNote(DetectionType type);
const char *why(const Detection &detection);
const char *lookup(const char *english);
const char *name(uint8_t language);
bool titleTap(uint32_t now);
void resetTaps();
void next();
uint16_t nextCodepoint(const char *&p);
int width(const char *text);
// Logical text is wrapped first; Hebrew is reordered separately for each line.
int draw(TFT_eSPI &t, const char *text, int x, int y, int w, int maxH, uint16_t color,
         bool translate = true, bool centered = false);
void button(TFT_eSPI &t, int x, int y, int w, int h, const char *label, bool selected = false);
} // namespace Lang
