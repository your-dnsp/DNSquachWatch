#pragma once
#include <stdint.h>

namespace ScanProfile {
enum Profile : uint8_t { STATIONARY=0, BALANCED=1, FAST=2, MAXIMUM=3, CUSTOM=4 };
struct Result {
    uint32_t wifiFrames=0, bleAdverts=0, sweeps=0, detections=0, drops=0;
    uint32_t minBlock=0;
};
void begin();
Profile current();
const char* name(Profile);
const char* description(Profile);
void select(Profile, uint32_t now=0);
uint16_t wifiCycleMs();
uint8_t bleShare();
uint16_t customWifiMs();
uint8_t customBleShare();
void adjustCustomWifi(int direction);
void adjustCustomBle(int direction);
void setCustomWifi(uint16_t milliseconds);
void setCustomBle(uint8_t percent);
uint8_t maximumMinutes();
void cycleMaximumMinutes();
void restore(Profile,uint16_t wifiMs,uint8_t ble,uint8_t maxMinutes);
void tick(uint32_t now, uint32_t wifiFrames, uint32_t bleAdverts,
          uint32_t sweeps, uint32_t detections, uint32_t drops, uint32_t largestBlock);
void startComparison(uint32_t now, uint32_t wifiFrames, uint32_t bleAdverts,
                     uint32_t sweeps, uint32_t detections, uint32_t drops);
void stopComparison();
bool comparing();
bool comparisonDone();
uint8_t comparisonProfile();
uint8_t comparisonPercent(uint32_t now);
const Result& result(uint8_t profile);
}
