// DNSquachWatch — deliberate, user-observed labels for radio findings.
#pragma once
#include "state.h"
#include <stddef.h>
#include <stdint.h>

namespace UserLabels {

constexpr uint8_t MAX_SUBTAG = 24;
constexpr uint8_t OTHER_TAG = 255;

struct Target {
    uint8_t mac[6]{};
    bool ble = true;
    DetectionType original = DetectionType::UNKNOWN;
    int8_t rssi = 0;
    uint8_t channel = 0;
    Confidence confidence = Confidence::LOW_CONF;
    MatchEvidence evidence = MatchEvidence::UNKNOWN;
    uint16_t signature = 0;
    char vendor[24]{};
    char name[24]{};
};

struct Label {
    uint8_t type = OTHER_TAG; // DetectionType value, or OTHER_TAG
    char subtag[MAX_SUBTAG + 1]{};
};

using ExportSink = bool (*)(const Target&, const Label&, uint32_t now);
void setExportSink(ExportSink sink);
void begin();
bool save(const Target& target, const Label& label, uint32_t now);
bool lookup(const uint8_t mac[6], Label& out);
uint8_t count();
bool at(uint8_t index,uint8_t mac[6],Label& out);
bool restore(const uint8_t mac[6],const Label& label);
void clearAll();
const char* typeName(uint8_t type);
void clearSession();

// Device implementation. It creates /User Labeled Device Findings,
// appends user-labeled-device-findings.csv and writes one text file per save.
bool storageExport(const Target& target, const Label& label, uint32_t now);
void storageWipe();

} // namespace UserLabels
