#include "field_tools.h"
#include <cstdio>
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SD.h>
#include <Arduino.h>
#endif
namespace Field {
static bool active = false, started = false;
static uint32_t startAt = 0;
static Telemetry data;
static const char *status = "Not connected. Requires compatible TX Backpack.";
#if defined(ARDUINO_ARCH_ESP32)
static bool bound = false;
static WiFiUDP udp;
static TelemetryConfig connection;
#endif
bool telemetryActive() {
    return active;
}
const char *telemetryStatus() {
    return status;
}
const Telemetry &telemetry() {
    return data;
}
void telemetryStop() {
#if defined(ARDUINO_ARCH_ESP32)
    bound = false;
    udp.stop();
    if (active)
        WiFi.disconnect(false);
    erase(connection.password, sizeof connection.password);
#endif
    active = false;
    started = false;
    status = "Disconnected; scanning resumes";
}
void telemetryStart() {
    if (active)
        return;
#if defined(ARDUINO_ARCH_ESP32)
    File f = SD.open("/dnsp-telemetry.txt", FILE_READ);
    if (!f) {
        status = "Missing /dnsp-telemetry.txt on microSD";
        return;
    }
    char b[256]{};
    size_t n = f.size();
    if (n >= sizeof b) {
        f.close();
        status = "Telemetry configuration too large";
        return;
    }
    size_t got = f.readBytes(b, n);
    f.close();
    bool valid = got == n && parseTelemetryConfig(b, n, connection);
    erase(b, sizeof b);
    if (!valid) {
        status = "Invalid telemetry configuration";
        return;
    }
    active = true;
    started = false;
    data = Telemetry{};
    status = "Connecting to your configured network";
#else
    status = "Simulator: radio unavailable; protocol tested separately";
#endif
}
void telemetryTick(uint32_t now) {
#if defined(ARDUINO_ARCH_ESP32)
    if (!active)
        return;
    if (!started) {
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.begin(connection.ssid, connection.password);
        erase(connection.password, sizeof connection.password);
        startAt = now;
        started = true;
    }
    if (WiFi.status() != WL_CONNECTED) {
        if (now - startAt > 20000) {
            telemetryStop();
            status = "Connection lost or timed out";
        }
        return;
    }
    if (!bound) {
        udp.stop();
        bound = udp.begin(connection.port);
        if (!bound) {
            telemetryStop();
            status = "Cannot open telemetry receiver";
            return;
        }
    }
    status = "Receiving only. Scanning paused. Unsigned telemetry.";
    for (int budget = 0; budget < 4; budget++) {
        int n = udp.parsePacket();
        if (n <= 0)
            break;
        IPAddress ip = udp.remoteIP();
        bool match = true;
        for (int i = 0; i < 4; i++)
            if (ip[i] != connection.source[i])
                match = false;
        uint8_t packet[512];
        if (!match || n > (int)sizeof packet) {
            udp.flush();
            data.rejected++;
            continue;
        }
        int got = udp.read(packet, n);
        if (got != n) {
            data.rejected++;
            continue;
        }
        decodeMavlink(packet, n, connection.system, now, data);
    }
#else
    (void)now;
#endif
}
bool exportPit() {
#if defined(ARDUINO_ARCH_ESP32)
    // Bounded replacement, no indefinite append; no user strings in CSV cells.
    File f = SD.open("/dnsp-fpv-pit.csv", FILE_WRITE);
    if (!f)
        return false;
    bool ok = f.print("pilot,band,channel,frequency_mhz,potential_overlap\n") > 0;
    for (int i = 0; i < 4; i++) {
        char row[96];
        bool overlap = false;
        for (int j = 0; j < 4; j++)
            overlap |= conflict(i, j);
        int n = snprintf(row, sizeof row, "%d,%c,%u,%u,%s\n", i + 1, "ABEFR"[config.bands[i]],
                         config.channels[i], frequency(config.bands[i], config.channels[i]),
                         overlap ? "yes" : "no");
        ok &= f.write((const uint8_t *)row, n) == (size_t)n;
    }
    f.flush();
    f.close();
    return ok;
#else
    return false;
#endif
}
} // namespace Field
