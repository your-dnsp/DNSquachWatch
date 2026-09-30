#pragma once
#include "detection.h"
#include <TFT_eSPI.h>
namespace FieldUI {
enum Page : uint8_t {
    HOME,
    EXTRA,
    PIT,
    DRONES,
    TELEMETRY,
    SENSORS,
    RULES,
    ACCESS,
    LANGUAGE,
    HELP,
    DISCOVER, DRONE_TOOLS, DRONE_DIAG, DRONE_CAPTURE, DRONE_LIMITS,
    SCREEN_LIGHT, RANDOMIZER, TIMER_COUNTER, POCKET_READER, RADIO_ACTIVITY,
    RULE_EVIDENCE, RULE_HISTORY, RULE_ABOUT,
    ALERT_HISTORY_SCAN, ALERT_HISTORY_SYSTEM, SCAN_PROFILES, SCAN_CUSTOM, SCAN_COMPARE
};
void openPage(Page);
#if !defined(ARDUINO_ARCH_ESP32)
uint32_t drawCount();
#endif
bool needsDraw(uint32_t now, int width, int height);
void open();
void cancelInput();
bool input(int x, int y, int w, int h, bool down, bool justDown, uint32_t now, const DetectionEngine &engine);
void openHelp();
void openLanguage();
void openAccessibility();
void openRuleHistory();
void openAlertHistory();
void openScanProfiles();
void openRules();
void draw(TFT_eSPI &t, uint32_t now, const DetectionEngine &engine);
bool tap(int x, int y, int w, int h, uint32_t now, const DetectionEngine &engine);
uint8_t currentPage();
bool keepsAwake();
} // namespace FieldUI
