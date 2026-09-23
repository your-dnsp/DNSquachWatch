#pragma once
#include "state.h"
#include "remote_id.h"
#include <stddef.h>
namespace Field {
struct Config {
    uint8_t language = 0;
    uint8_t hebrew = false, contrast = false, reduced = false, left = false, large = false;
    uint8_t quietPrefix = false, compositeOnly = false;
    uint8_t bands[4]{}, channels[4]{}; // 0=unassigned; bands A B E F R
    uint8_t sensors[4][6]{};
    uint8_t sensorOn[4]{};
    uint8_t muted[4][6]{};
    uint8_t muteOn[4]{};
};
extern Config config;
void erase(void *data, size_t size); // volatile clearing for temporary credentials
void begin();
void save();
void reset();
void wipePrivate();
bool allowAlert(const Detection &d);
const char *fpvName(const char *ssid);
uint16_t frequency(uint8_t band, uint8_t channel);
bool conflict(uint8_t a, uint8_t b);
struct Sensor {
    bool used = false;
    uint8_t mac[6]{};
    uint32_t at = 0;
    int16_t temperature = 0, previous = 0;
    uint16_t humidity = 0;
    uint8_t battery = 0, fields = 0;
    bool trend = false;
};
bool decodeSensor(const uint8_t *ad, size_t n, Sensor &out);
struct Aircraft {
    bool used = false, wifi = false;
    uint8_t mac[6]{};
    RemoteId::Info info;
};
bool observeBle(const uint8_t *mac, const uint8_t *ad, size_t n, uint32_t now);
bool observeWifi(const uint8_t *frame, size_t n, uint32_t now);
void tick();
const Aircraft &aircraft(uint8_t index);
const Sensor &sensor(uint8_t index);
const Sensor &discovery(uint8_t index);
uint32_t dropped();
struct Telemetry {
    bool heartbeat = false, position = false, battery = false, link = false;
    uint32_t heartbeatAt = 0, positionAt = 0, batteryAt = 0, linkAt = 0;
    float lat = 0, lon = 0, alt = 0;
    uint16_t millivolts = 0;
    int8_t remaining = -1;
    uint8_t rssi = 0, remoteRssi = 0;
    uint32_t accepted = 0, rejected = 0;
};
bool decodeMavlink(const uint8_t *data, size_t n, uint8_t systemId, uint32_t now, Telemetry &out);
// Dedicated, explicitly opened own-equipment WiFi mode. Never sends flight commands.
struct TelemetryConfig {
    char ssid[33]{}, password[64]{};
    uint8_t source[4]{}, system = 1;
    uint16_t port = 14550;
};
bool parseTelemetryConfig(const char *text, size_t len, TelemetryConfig &out);
void telemetryStart();
void telemetryStop();
void telemetryTick(uint32_t now);
bool telemetryActive();
const char *telemetryStatus();
const Telemetry &telemetry();
bool exportPit();
} // namespace Field
