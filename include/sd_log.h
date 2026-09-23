// SquachWatch-CYD — optional SD card event log
// If the SD card is mounted at boot, each Detection is appended to
// /squachwatch-YYYYMMDD.log as one CSV line.
// If the card is absent, every call is a silent no-op.
#pragma once
#include <Arduino.h>
#include "state.h"

class SdLog {
public:
    void describe(char* out, size_t cap);
    void logPressure(uint32_t alerts, uint32_t ble);
    bool begin();              // returns true if card mounted
    bool ready() const { return _ready; }
    void logEvent(const Detection& d);
    bool safeEnd(); // sync block device, unmount; false if writes could not be confirmed
    void tick();               // flush / housekeeping (called from loop)
    // Deletes every squachwatch log file on the card. For the security wipe --
    // the phrase and the ignore list live in NVS, but the detection history a
    // wipe must also erase is here. A no-op when no card is mounted.
    void wipe();
private:
    uint32_t _pressureAt = 0, _loggedAlerts = 0, _loggedBle = 0;
    uint8_t  _drive = 255;
    uint32_t _writeErrors = 0;
    bool     _ready = false;
    uint32_t _lastFlush = 0;
    char     _filename[24] = {0};
    void     openDaily();
};

