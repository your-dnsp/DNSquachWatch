#include "field_tools.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <Preferences.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#else
#include <mutex>
#endif
namespace Field {
Config config;
void erase(void *data, size_t size) {
    volatile uint8_t *p = (volatile uint8_t *)data;
    while (size--)
        *p++ = 0;
}
namespace {
struct EraseOnExit {
    void *data;
    size_t size;
    ~EraseOnExit() {
        erase(data, size);
    }
};
} // namespace

static const uint16_t frequencies[5][8] = {{5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725},
                                           {5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866},
                                           {5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945},
                                           {5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880},
                                           {5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917}};
uint16_t frequency(uint8_t b, uint8_t c) {
    return b < 5 && c >= 1 && c <= 8 ? frequencies[b][c - 1] : 0;
}
bool conflict(uint8_t a, uint8_t b) {
    if (a >= 4 || b >= 4 || a == b)
        return false;
    int x = frequency(config.bands[a], config.channels[a]),
        y = frequency(config.bands[b], config.channels[b]);
    return x && y && abs(x - y) < 40;
}
void begin() {
    Preferences p;
    if (!p.begin("dnsp-field", true))
        return;
    Config c;
    if (p.getBytesLength("v1") == sizeof c && p.getBytes("v1", &c, sizeof c) == sizeof c) {
        // Validate stored scalar values; bool representation is generated only by this firmware.
        if (c.language < 7 && c.hebrew <= 1 && c.contrast <= 1 && c.reduced <= 1 && c.left <= 1 &&
            c.large <= 1 && c.quietPrefix <= 1 && c.compositeOnly <= 1) {
            config = c;
            for (int i = 0; i < 4; i++) {
                if (config.bands[i] > 4)
                    config.bands[i] = 0;
                if (config.channels[i] > 8)
                    config.channels[i] = 0;
                if (config.sensorOn[i] > 1)
                    config.sensorOn[i] = 0;
                if (config.muteOn[i] > 1)
                    config.muteOn[i] = 0;
            }
            if (config.language == 6)
                config.hebrew = true;
        }
    }
    p.end();
}
void save() {
    Preferences p;
    if (p.begin("dnsp-field", false)) {
        p.putBytes("v1", &config, sizeof config);
        p.end();
    }
}
bool allowAlert(const Detection &d) {
    if (config.quietPrefix && d.evidence == MatchEvidence::OUI)
        return false;
    if (config.compositeOnly) {
        unsigned n = 0;
        uint16_t bits = d.evidenceBits & 0x013f;
        while (bits) {
            n += bits & 1;
            bits >>= 1;
        }
        if (d.evidence != MatchEvidence::RESEARCH_COMPOSITE || n < 2)
            return false;
    }
    for (int i = 0; i < 4; i++)
        if (config.muteOn[i] && !memcmp(config.muted[i], d.mac, 6))
            return false;
    return true;
}
const char *fpvName(const char *s) {
    if (!s)
        return nullptr;
    if (!strcmp(s, "ExpressLRS RX"))
        return "ELRS receiver setup";
    if (!strcmp(s, "ExpressLRS TX"))
        return "ELRS transmitter setup";
    const char *prefix = "ExpressLRS TX Backpack ";
    size_t z = strlen(prefix);
    if (!strncmp(s, prefix, z) && strlen(s) == z + 6) {
        for (size_t i = z; i < z + 6; i++)
            if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'A' && s[i] <= 'F') ||
                  (s[i] >= 'a' && s[i] <= 'f')))
                return nullptr;
        return "ELRS TX Backpack";
    }
    return nullptr;
}
static uint16_t u16(const uint8_t *p) {
    return p[0] | (uint16_t)p[1] << 8;
}
static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
bool decodeSensor(const uint8_t *p, size_t n, Sensor &out) {
    if (!p || n > 255)
        return false;
    Sensor v;
    bool found = false;
    for (size_t i = 0; i < n;) {
        size_t z = p[i];
        if (!z)
            break;
        if (z > n - i - 1)
            return false;
        if (z >= 4 && p[i + 1] == 0x16 && p[i + 2] == 0xd2 && p[i + 3] == 0xfc) {
            uint8_t info = p[i + 4];
            if ((info >> 5) != 2 || (info & 0x1b))
                return false; // unencrypted BTHome v2, reserved bits clear
            size_t end = i + 1 + z;
            for (size_t j = i + 5; j < end;) {
                uint8_t id = p[j++];
                size_t width = (id == 0 || id == 1 || id == 0x2e) ? 1
                               : (id == 2 || id == 3)             ? 2
                                                                  : 0;
                if (!width || width > end - j)
                    return false; // unknown object: never guess its size
                if (id == 1) {
                    if (p[j] > 100)
                        return false;
                    v.battery = p[j];
                    v.fields |= 4;
                }
                if (id == 2) {
                    v.temperature = (int16_t)u16(p + j);
                    v.fields |= 1;
                }
                if (id == 3) {
                    v.humidity = u16(p + j);
                    if (v.humidity > 10000)
                        return false;
                    v.fields |= 2;
                }
                if (id == 0x2e) {
                    if (p[j] > 100)
                        return false;
                    v.humidity = p[j] * 100;
                    v.fields |= 2;
                }
                j += width;
            }
            found = true;
        }
        i += z + 1;
    }
    if (!found || !v.fields)
        return false;
    out = v;
    return true;
}
struct Event {
    uint8_t kind = 0, mac[6]{};
    uint32_t now = 0;
    uint16_t len = 0;
    uint8_t data[228]{};
};
static Event events[6];
static uint8_t head = 0, count = 0;
static uint32_t drops = 0;
static Aircraft planes[4];
static Sensor selected[4], seen[4];
#if defined(ARDUINO_ARCH_ESP32)
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
struct Guard {
    Guard() {
        portENTER_CRITICAL(&mux);
    }
    ~Guard() {
        portEXIT_CRITICAL(&mux);
    }
};
#else
static std::mutex mux;
struct Guard {
    std::lock_guard<std::mutex> lock{mux};
};
#endif
static void post(uint8_t kind, const uint8_t *mac, const uint8_t *p, size_t n, uint32_t now) {
    if (!mac || !p || n > 228)
        return;
    Guard g;
    if (count == 6) {
        drops++;
        return;
    }
    Event &e = events[(head + count) % 6];
    e.kind = kind;
    memcpy(e.mac, mac, 6);
    e.now = now;
    e.len = n;
    memcpy(e.data, p, n);
    count++;
}
bool observeBle(const uint8_t *mac, const uint8_t *p, size_t n, uint32_t now) {
    if (!mac || !p || n > 228)
        return false;
    RemoteId::Info r;
    Sensor sensor;
    if (RemoteId::merge(p, (uint8_t)n, r, now)) {
        post(0, mac, p, n, now);
        return true;
    }
    if (decodeSensor(p, n, sensor))
        post(2, mac, p, n, now);
    return false;
}

bool observeWifi(const uint8_t *f, size_t n, uint32_t now) {
    const uint8_t *p;
    size_t z;
    if (!RemoteId::wifiPayload(f, n, p, z))
        return false;
    post(1, f + 10, p, z, now);
    return true;
}
void tick() {
    for (int budget = 0; budget < 6; budget++) {
        Event e;
        {
            Guard g;
            if (!count)
                break;
            e = events[head];
            head = (head + 1) % 6;
            count--;
        }
        if (e.kind < 2) {
            int slot = -1;
            for (int i = 0; i < 4; i++)
                if (planes[i].used && planes[i].wifi == (e.kind == 1) &&
                    !memcmp(planes[i].mac, e.mac, 6)) {
                    slot = i;
                    break;
                }
            if (slot < 0) {
                uint32_t age = 0;
                slot = 0;
                for (int i = 0; i < 4; i++) {
                    if (!planes[i].used) {
                        slot = i;
                        break;
                    }
                    if (e.now - planes[i].info.at >= age) {
                        age = e.now - planes[i].info.at;
                        slot = i;
                    }
                }
                planes[slot] = Aircraft{};
            }
            auto &a = planes[slot];
            a.used = true;
            a.wifi = e.kind == 1;
            memcpy(a.mac, e.mac, 6);
            if (e.kind == 1)
                RemoteId::mergePack(e.data, e.len, a.info, e.now);
            else
                RemoteId::merge(e.data, (uint8_t)e.len, a.info, e.now);
            if (a.info.haveBasic && a.info.serial[0])
                for (int j = 0; j < 4; j++)
                    if (j != slot && planes[j].used && planes[j].wifi == a.wifi &&
                        planes[j].info.haveBasic && planes[j].info.idType == a.info.idType &&
                        e.now - planes[j].info.basicAt < 15000 &&
                        !strcmp(planes[j].info.serial, a.info.serial)) {
                        a.info.quality |= 32;
                        planes[j].info.quality |= 32;
                    }
        } else {
            Sensor s;
            if (!decodeSensor(e.data, e.len, s))
                continue;
            s.used = true;
            s.at = e.now;
            memcpy(s.mac, e.mac, 6);
            int k = -1;
            for (int i = 0; i < 4; i++)
                if (seen[i].used && !memcmp(seen[i].mac, e.mac, 6)) {
                    k = i;
                    break;
                }
            if (k < 0) {
                uint32_t age = 0;
                k = 0;
                for (int i = 0; i < 4; i++) {
                    if (!seen[i].used) {
                        k = i;
                        break;
                    }
                    if (e.now - seen[i].at >= age) {
                        age = e.now - seen[i].at;
                        k = i;
                    }
                }
            }
            seen[k] = s;
            for (int i = 0; i < 4; i++)
                if (config.sensorOn[i] && !memcmp(config.sensors[i], e.mac, 6)) {
                    if ((s.fields & 1) && (selected[i].fields & 1) && selected[i].used) {
                        s.previous = selected[i].temperature;
                        s.trend = true;
                    }
                    selected[i] = s;
                }
        }
    }
}
int associatedAircraft(uint8_t index, uint32_t now) {
    if (index >= 4) return -1;
    const auto &a = planes[index];
    if (!a.used || !a.info.haveBasic || !a.info.serial[0] || now-a.info.basicAt > 15000) return -1;
    for (int j=0;j<4;j++) {
        const auto &b=planes[j];
        if (j!=index && b.used && a.wifi!=b.wifi && b.info.haveBasic &&
            now-b.info.basicAt<=15000 && a.info.idType==b.info.idType &&
            !strcmp(a.info.serial,b.info.serial)) return j;
    }
    return -1;
}
bool identityConflict(uint8_t index, uint32_t now) {
    int j=associatedAircraft(index,now); if(j<0) return false;
    const auto &a=planes[index].info; const auto &b=planes[j].info;
    if(a.uaType!=b.uaType) return true;
    if(!a.haveLoc || !b.haveLoc || now-a.locAt>3000 || now-b.locAt>3000) return false;
    float dy=(a.lat-b.lat)*111320.0f, dl=a.lon-b.lon;
    if(dl>180)dl-=360; if(dl< -180)dl+=360;
    float dx=dl*111320.0f*cosf(a.lat*0.0174532925f);
    // Association is only a display hint, never authentication. Allow flight movement.
    return dx*dx+dy*dy > 1000.0f*1000.0f;
}
const Aircraft &aircraft(uint8_t i) {
    return planes[i < 4 ? i : 0];
}
const Sensor &sensor(uint8_t i) {
    return selected[i < 4 ? i : 0];
}
const Sensor &discovery(uint8_t i) {
    return seen[i < 4 ? i : 0];
}
uint32_t dropped() {
    Guard g;
    return drops;
}
void reset() {
    Guard g;
    head = count = 0;
    drops = 0;
    for (auto &x : planes)
        x = Aircraft{};
    for (auto &x : selected)
        x = Sensor{};
    for (auto &x : seen)
        x = Sensor{};
}
void wipePrivate() {
    for (int i = 0; i < 4; i++) {
        config.sensorOn[i] = config.muteOn[i] = 0;
        memset(config.sensors[i], 0, 6);
        memset(config.muted[i], 0, 6);
    }
    reset();
    save();
}
static void crcByte(uint16_t &crc, uint8_t b) {
    uint8_t t = b ^ (uint8_t)crc;
    t ^= t << 4;
    crc = (crc >> 8) ^ ((uint16_t)t << 8) ^ ((uint16_t)t << 3) ^ (t >> 4);
}
bool decodeMavlink(const uint8_t *p, size_t n, uint8_t sys, uint32_t now, Telemetry &out) {
    if (!p || !sys || n > 1024)
        return false;
    bool any = false;
    for (size_t i = 0; i < n;) {
        bool v2 = p[i] == 0xfd;
        if (p[i] != 0xfe && !v2) {
            i++;
            continue;
        }
        size_t h = v2 ? 10 : 6;
        if (n - i < h + 2) {
            out.rejected++;
            break;
        }
        size_t z = p[i + 1], total = h + z + 2 + (v2 && (p[i + 2] & 1) ? 13 : 0);
        if (total > n - i) {
            out.rejected++;
            break;
        }
        uint32_t id =
            v2 ? (uint32_t)p[i + 7] | (uint32_t)p[i + 8] << 8 | (uint32_t)p[i + 9] << 16 : p[i + 5];
        uint8_t sender = p[i + (v2 ? 5 : 3)];
        uint8_t extra = 0;
        size_t minLen = 0;
        switch (id) {
        case 0:
            extra = 50;
            minLen = 9;
            break;
        case 1:
            extra = 124;
            minLen = 31;
            break;
        case 24:
            extra = 24;
            minLen = 30;
            break;
        case 33:
            extra = 104;
            minLen = 28;
            break;
        case 109:
            extra = 185;
            minLen = 9;
            break;
        default:
            i += total;
            continue;
        }
        if (sender != sys || (v2 && p[i + 2]) || !z || (!v2 && z != minLen) ||
            z > minLen + (id == 24 ? 22 : 0)) {
            out.rejected++;
            i += total;
            continue;
        }
        uint16_t crc = 0xffff;
        for (size_t j = i + 1; j < i + h + z; j++)
            crcByte(crc, p[j]);
        crcByte(crc, extra);
        if (crc != u16(p + i + h + z)) {
            out.rejected++;
            i += total;
            continue;
        }
        uint8_t d[52]{};
        memcpy(d, p + i + h, z); // MAVLink2 zero-truncated trailing bytes
        if (id == 0) {
            if (d[8] != 3 || d[4] == 6) {
                out.rejected++;
                i += total;
                continue;
            }
            out.heartbeat = true;
            out.heartbeatAt = now;
        }
        if (id == 1) {
            uint16_t mv = u16(d + 14);
            if (mv != 65535) {
                out.millivolts = mv;
                out.remaining = (int8_t)d[30];
                if (out.remaining < -1 || out.remaining > 100)
                    out.remaining = -1;
                out.battery = true;
                out.batteryAt = now;
            }
        }
        if (id == 24 || id == 33) {
            size_t o = id == 24 ? 8 : 4;
            int32_t la = (int32_t)u32(d + o), lo = (int32_t)u32(d + o + 4);
            bool fix = id == 33 || (d[28] >= 3 && d[28] <= 8);
            if (fix && la >= -900000000 && la <= 900000000 && lo >= -1800000000 &&
                lo <= 1800000000) {
                out.lat = la / 1e7f;
                out.lon = lo / 1e7f;
                out.alt = (int32_t)u32(d + o + 8) / 1000.f;
                out.position = true;
                out.positionAt = now;
            }
        }
        if (id == 109) {
            out.rssi = d[4];
            out.remoteRssi = d[5];
            out.link = true;
            out.linkAt = now;
        }
        out.accepted++;
        any = true;
        i += total;
    }
    return any;
}
} // namespace Field

namespace Field {
bool parseTelemetryConfig(const char *text, size_t len, TelemetryConfig &out) {
    if (!text || len >= 256 || !len)
        return false;
    TelemetryConfig c;
    EraseOnExit clearConfig{&c, sizeof c};
    unsigned fields = 0;
    for (size_t i = 0; i < len;) {
        size_t j = i;
        while (j < len && text[j] != '\n') {
            if (text[j] == '\r' && (j + 1 == len || text[j + 1] == '\n')) {
                j++;
                break;
            }
            if (text[j] < 32 || text[j] > 126)
                return false;
            j++;
        }
        size_t n = j - i;
        if (n && text[i + n - 1] == '\r')
            n--;
        if (n) {
            const char *e = (const char *)memchr(text + i, '=', n);
            if (!e)
                return false;
            size_t k = e - (text + i), z = n - k - 1;
            const char *v = e + 1;
            char value[64]{};
            EraseOnExit clearValue{value, sizeof value};
            if (z >= sizeof value)
                return false;
            memcpy(value, v, z);
            unsigned bit = 0;
            if (k == 4 && !memcmp(text + i, "ssid", 4)) {
                bit = 1;
                if (!z || z > 32)
                    return false;
                memcpy(c.ssid, value, z + 1);
            } else if (k == 8 && !memcmp(text + i, "password", 8)) {
                bit = 2;
                if (z && z < 8)
                    return false;
                memcpy(c.password, value, z + 1);
            } else if (k == 6 && !memcmp(text + i, "source", 6)) {
                bit = 4;
                size_t pos = 0;
                for (int octet = 0; octet < 4; octet++) {
                    unsigned valueNum = 0, digits = 0;
                    while (pos < z && value[pos] >= '0' && value[pos] <= '9') {
                        if (++digits > 3)
                            return false;
                        valueNum = valueNum * 10 + (value[pos++] - '0');
                    }
                    if (!digits || valueNum > 255)
                        return false;
                    c.source[octet] = valueNum;
                    if (octet < 3) {
                        if (pos >= z || value[pos++] != '.')
                            return false;
                    }
                }
                if (pos != z || !c.source[0] || c.source[0] >= 224 || !c.source[3] ||
                    c.source[3] == 255)
                    return false;
            } else if ((k == 6 && !memcmp(text + i, "system", 6)) ||
                       (k == 4 && !memcmp(text + i, "port", 4))) {
                bit = k == 6 ? 8 : 16;
                if (!z)
                    return false;
                for (size_t t = 0; t < z; t++)
                    if (value[t] < '0' || value[t] > '9')
                        return false;
                char *end;
                unsigned long num = strtoul(value, &end, 10);
                if (*end || !num || num > (bit == 8 ? 255 : 65535))
                    return false;
                if (bit == 8)
                    c.system = num;
                else
                    c.port = num;
            } else
                return false;
            if (fields & bit)
                return false;
            fields |= bit;
        }
        i = j + 1;
    }
    if (fields != 31)
        return false;
    out = c;
    return true;
}
} // namespace Field
