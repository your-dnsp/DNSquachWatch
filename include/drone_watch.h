#pragma once
#include "state.h"
#include <stddef.h>
#include <stdint.h>
namespace DroneWatch {
enum class Verdict : uint8_t { NONE, CLUE, UNSUPPORTED, MALFORMED, DECODED };
enum class Capture : uint8_t { IDLE, STARTING, RECORDING, SAVING, DONE, ERROR };
struct Stats {
    uint32_t wifi=0, ble=0, decoded=0, clues=0, unsupported=0, malformed=0;
    uint32_t channelErrors=0, saved=0, dropped=0, truncated=0, started=0, elapsed=0;
    uint8_t channel=0;
    Capture capture=Capture::IDLE;
    Verdict last=Verdict::NONE;
};
struct Record {
    uint32_t at=0;
    uint16_t original=0, length=0;
    uint8_t mac[6]{}, channel=0;
    int8_t rssi=0;
    bool wifi=false;
    Verdict verdict=Verdict::NONE;
    uint8_t data[1024]{};
};
// Only validated payloads establish a Remote ID alert. Metadata lists are optional.
inline void applyDecodedBle(Detection &d, bool valid) {
    if(valid) {d.type=DetectionType::DRONE;d.conf=Confidence::MED_CONF;
        d.evidence=MatchEvidence::BLE_REMOTE_ID;d.signature=0xfffa;d.evidenceBits=0;}
}
Verdict inspect(bool wifi,const uint8_t *data,size_t length);
const char *verdictText(Verdict v);
void observe(bool wifi,const uint8_t *mac,const uint8_t *data,size_t length,int8_t rssi,uint8_t channel,uint32_t now);
Stats stats();
void setFocused(bool enabled,uint32_t now);
bool focused();
bool wifiPhase(uint32_t now);
uint8_t preferredChannel(uint32_t now);
void channelResult(uint8_t actual,bool success);
bool startCapture(uint32_t now); // caller confirms raw identifier/position collection
void stopCapture();
void captureReady(); // called after PREPARING has been drawn
void tick(uint32_t now);
bool settled();
void wipe();
// Bounded JSONL records, captured bytes are hex; no untrusted strings are interpolated.
bool formatRecord(const Record &r,char *out,size_t capacity);
// Injected sink permits failure and queue tests without a radio or SD card.
using Sink=bool (*)(const char *line,bool begin);
void setSink(Sink sink);
}
