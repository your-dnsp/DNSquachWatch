#include "field_tools.h"
#include "test_util.h"
#include <cstring>
#include <vector>
using namespace Field;
static void put32(uint8_t *p, int32_t x) {
    for (int i = 0; i < 4; i++)
        p[i] = (uint32_t)x >> (8 * i);
}
static void crcByte(uint16_t &crc, uint8_t b) {
    uint8_t tmp = b ^ (uint8_t)crc;
    tmp ^= tmp << 4;
    crc = (crc >> 8) ^ ((uint16_t)tmp << 8) ^ ((uint16_t)tmp << 3) ^ (tmp >> 4);
}
static std::vector<uint8_t> mav(uint8_t id, uint8_t extra, const std::vector<uint8_t> &body,
                                bool v2 = false) {
    std::vector<uint8_t> p =
        v2 ? std::vector<uint8_t>{0xfd, (uint8_t)body.size(), 0, 0, 0, 1, 1, id, 0, 0}
           : std::vector<uint8_t>{0xfe, (uint8_t)body.size(), 0, 1, 1, id};
    p.insert(p.end(), body.begin(), body.end());
    uint16_t crc = 65535;
    for (size_t i = 1; i < p.size(); i++)
        crcByte(crc, p[i]);
    crcByte(crc, extra);
    p.push_back(crc);
    p.push_back(crc >> 8);
    return p;
}
static std::vector<uint8_t> beacon() {
    std::vector<uint8_t> p(36);
    p[0] = 0x80;
    p[10] = 2;
    p.push_back(221);
    p.push_back(33);
    p.insert(p.end(), {0xfa, 0x0b, 0xbc, 0x0d, 0, 0xf2, 25, 1, 2, 0x12});
    for (int i = 0; i < 23; i++)
        p.push_back(i < 4 ? 'A' : 0);
    return p;
}
int main() {
    suite("FPV clue boundaries and channel planning");
    ck("exact ELRS RX", fpvName("ExpressLRS RX") != nullptr);
    ck("no generic Espressif", !fpvName("ESP_ABC"));
    ck("no substring false positive", !fpvName("Not ExpressLRS RX"));
    ck("backpack suffix", fpvName("ExpressLRS TX Backpack a1B2c3") != nullptr);
    ck("invalid suffix rejected", !fpvName("ExpressLRS TX Backpack zzzzzz"));
    ck("unassigned channel", frequency(0, 0) == 0);
    ck("Raceband 1", frequency(4, 1) == 5658);
    ck("invalid band", frequency(5, 1) == 0);
    config = Config{};
    config.channels[0] = config.channels[1] = 1;
    ck("duplicate channel warning", conflict(0, 1));
    config.channels[1] = 8;
    ck("wide separation", !conflict(0, 1));
    ck("self not conflicting", !conflict(0, 0));
    suite("WiFi Remote ID validation");
    auto b = beacon();
    const uint8_t *body;
    size_t len;
    ck("beacon accepted", RemoteId::wifiPayload(b.data(), b.size(), body, len));
    RemoteId::Info r;
    ck("pack decoded", RemoteId::mergePack(body, len, r, 100) && r.haveBasic);
    ck("serial sanitized", strcmp(r.serial, "AAAA") == 0);
    for (size_t n = 0; n < b.size(); n++) {
        const uint8_t *q;
        size_t z;
        ck("truncated frame rejected", !RemoteId::wifiPayload(b.data(), n, q, z));
    }
    auto bad = b;
    bad[38] = 0;
    ck("unrelated OUI rejected", !RemoteId::wifiPayload(bad.data(), bad.size(), body, len));
    bad = b;
    bad[45] = 10;
    ck("oversized message count rejected",
       !RemoteId::wifiPayload(bad.data(), bad.size(), body, len));
    std::vector<uint8_t> nan(30);
    nan[0] = 0xd0;
    uint8_t da[] = {0x51, 0x6f, 0x9a, 1, 0, 0}, nh[] = {4, 9, 0x50, 0x6f, 0x9a, 0x13};
    memcpy(nan.data() + 4, da, 6);
    memcpy(nan.data() + 24, nh, 6);
    nan.insert(nan.end(), {3, 39, 0, 0x88, 0x69, 0x19, 0x9d, 0x92, 9, 1, 0, 0x10, 29, 0});
    nan.insert(nan.end(), b.begin() + 43, b.end());
    ck("NAN accepted", RemoteId::wifiPayload(nan.data(), nan.size(), body, len));
    suite("Remote ID consistency, freshness and unavailable fix");
    uint8_t pack[28] = {0xf2, 25, 1, 0x12};
    put32(pack + 8, 400000000);
    put32(pack + 12, -740000000);
    r = RemoteId::Info{};
    ck("initial position", RemoteId::mergePack(pack, 28, r, 1000) && r.haveLoc);
    put32(pack + 8, 410000000);
    ck("jump flagged", RemoteId::mergePack(pack, 28, r, 2000) && (r.quality & 2));
    uint32_t locAt = r.locAt;
    pack[3] = 2;
    pack[4] = 0x12;
    pack[5] = 'A';
    RemoteId::mergePack(pack, 28, r, 30000);
    ck("basic ID does not refresh position", r.locAt == locAt);
    r.quality = 0;
    ck("stale field warning", strstr(RemoteId::qualityText(r, 30000), "stale"));
    pack[3] = 0x12;
    put32(pack + 8, 0);
    put32(pack + 12, 0);
    RemoteId::mergePack(pack, 28, r, 31000);
    ck("no fix preserves last known", r.noFix && r.locAt == locAt);
    put32(pack + 8, 1000000000);
    RemoteId::mergePack(pack, 28, r, 32000);
    ck("invalid coordinates flagged", r.quality & 1);
    suite("BTHome boundaries and selection");
    uint8_t ad[] = {13, 0x16, 0xd2, 0xfc, 0x40, 0, 8, 1, 92, 2, 0xd0, 7, 0x2e, 45};
    Sensor s;
    ck("BTHome v2 decoded", decodeSensor(ad, sizeof ad, s));
    ck("measurements", s.temperature == 2000 && s.humidity == 4500 && s.battery == 92);
    ad[4] = 0x41;
    ck("encrypted data rejected", !decodeSensor(ad, sizeof ad, s));
    ad[4] = 0x40;
    ad[7] = 0xff;
    ck("unknown object rejected", !decodeSensor(ad, sizeof ad, s));
    ad[7] = 1;
    ck("truncation", !decodeSensor(ad, sizeof ad - 1, s));
    reset();
    config = Config{};
    uint8_t mac[] = {2, 1, 2, 3, 4, 5};
    observeBle(mac, ad, sizeof ad, 10);
    tick();
    ck("discovered but not selected", discovery(0).used && !sensor(0).used);
    memcpy(config.sensors[0], mac, 6);
    config.sensorOn[0] = true;
    observeBle(mac, ad, sizeof ad, 20);
    tick();
    ck("chosen sensor populated", sensor(0).used && sensor(0).at == 20);
    for (int i = 0; i < 20; i++)
        observeBle(mac, ad, sizeof ad, 30);
    ck("bounded queue drops", dropped() == 14);
    tick();
    suite("Alert rules preserve logging eligibility separately");
    Detection d{};
    d.evidence = MatchEvidence::OUI;
    config.quietPrefix = true;
    ck("prefix quiet", !allowAlert(d));
    config.quietPrefix = false;
    config.compositeOnly = true;
    d.evidence = MatchEvidence::RESEARCH_COMPOSITE;
    d.evidenceBits = 64 | 1;
    ck("import marker not independent clue", !allowAlert(d));
    d.evidenceBits = 1 | 2;
    ck("two evidence types", allowAlert(d));
    config.compositeOnly = false;
    config.muteOn[0] = true;
    memcpy(config.muted[0], d.mac, 6);
    ck("owned device quiet", !allowAlert(d));
    suite("Strict telemetry configuration");
    TelemetryConfig c;
    const char *cfg =
        "ssid=ExpressLRS TX Backpack 123456\npassword=\nsource=10.0.0.1\nsystem=1\nport=14550\n";
    ck("valid explicit configuration",
       parseTelemetryConfig(cfg, strlen(cfg), c) && c.port == 14550);
    ck("no missing pinning", !parseTelemetryConfig("ssid=x\n", 7, c));
    const char *dup = "ssid=x\nssid=y\npassword=\nsource=10.0.0.1\nsystem=1\nport=14550\n";
    ck("duplicate key rejected", !parseTelemetryConfig(dup, strlen(dup), c));
    const char *crlf = "ssid=x\r\npassword=\r\nsource=10.0.0.1\r\nsystem=1\r\nport=14550\r\n";
    ck("Windows line endings accepted", parseTelemetryConfig(crlf, strlen(crlf), c));
    const char *overflow = "ssid=x\npassword=\nsource=4294967306.0.0.1\nsystem=1\nport=14550\n";
    ck("IPv4 integer overflow rejected", !parseTelemetryConfig(overflow, strlen(overflow), c));
    suite("MAVLink checksums, sender, truncation and freshness");
    std::vector<uint8_t> hb(9);
    hb[4] = 2;
    hb[8] = 3;
    auto packet = mav(0, 50, hb);
    Telemetry t;
    ck("heartbeat v1", decodeMavlink(packet.data(), packet.size(), 1, 100, t) && t.heartbeat);
    ck("sender rejected", !decodeMavlink(packet.data(), packet.size(), 2, 200, t));
    packet[8] ^= 1;
    ck("CRC rejected", !decodeMavlink(packet.data(), packet.size(), 1, 200, t));
    packet = mav(0, 50, hb, true);
    ck("heartbeat v2", decodeMavlink(packet.data(), packet.size(), 1, 300, t));
    packet[2] = 1;
    ck("signed packet not accepted without verification",
       !decodeMavlink(packet.data(), packet.size(), 1, 400, t));
    packet = mav(0, 50, hb);
    for (size_t n = 0; n < packet.size(); n++)
        ck("truncated MAVLink rejected", !decodeMavlink(packet.data(), n, 1, 500, t));
    std::vector<uint8_t> st(31);
    st[14] = 0x80;
    st[15] = 0x0c;
    st[30] = 75;
    packet = mav(1, 124, st);
    ck("battery decoded", decodeMavlink(packet.data(), packet.size(), 1, 600, t) &&
                              t.millivolts == 3200 && t.remaining == 75 && t.heartbeatAt == 300);
    std::vector<uint8_t> pos(28);
    put32(pos.data() + 4, 410000000);
    put32(pos.data() + 8, -730000000);
    put32(pos.data() + 12, 120000);
    packet = mav(33, 104, pos);
    ck("position decoded",
       decodeMavlink(packet.data(), packet.size(), 1, 700, t) && t.position && t.alt == 120);
    ck("location freshness separate", t.batteryAt == 600 && t.positionAt == 700);
    suite("Malformed radio corpus");
    uint32_t seed = 42;
    uint8_t bytes[512];
    for (int k = 0; k < 15000; k++) {
        for (auto &x : bytes) {
            seed = seed * 1664525 + 1013904223;
            x = seed >> 24;
        }
        size_t n = seed % sizeof bytes;
        RemoteId::wifiPayload(bytes, n, body, len);
        RemoteId::mergePack(bytes, n, r, k);
        decodeSensor(bytes, n, s);
        decodeMavlink(bytes, n, 1, k, t);
        parseTelemetryConfig((const char *)bytes, n, c);
    }
    ck("15000 malformed inputs", true);
    return report();
}
