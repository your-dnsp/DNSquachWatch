#pragma once
#include "detection.h"
#include <stdint.h>

namespace SketchyRule {
struct Endpoint {
    DetectionType type = DetectionType::UNKNOWN;
    uint8_t mac[6]{};
    int8_t rssi = 0;
    uint8_t channel = 0;
    uint16_t hits = 0;
    uint32_t at = 0;
    uint32_t locationKey=0;
    char name[20]{};
    char vendor[16]{};
};
struct Incident {
    uint32_t id = 0;
    Endpoint alpr{};
    Endpoint deauth{};
    uint32_t gapSeconds = 0;
    bool sdExported = false;
};

void begin();
void tick(const DetectionEngine& engine, uint32_t now);
bool takeAlert(Incident& out);
bool enabled();
void toggle();
void setEnabled(bool);
uint8_t count();
bool recent(uint8_t newestIndex, Incident& out);
void storageWipe();
}
