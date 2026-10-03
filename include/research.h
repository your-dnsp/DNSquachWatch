#pragma once
#include "state.h"
#include <stddef.h>
namespace Research {
constexpr uint32_t BUILTIN_VERSION = 20260923;
constexpr size_t PAYLOAD_CAP = 96, PACK_CAP = 4096;
enum class Profile : uint8_t { BALANCED, BLUETOOTH, WIFI };
enum class Verdict : uint8_t { UNREVIEWED, VISUAL, SUSPECTED, FALSE_POSITIVE };
enum Bits : uint16_t { COMPANY=1, SERVICE=2, NAME=4, ACCESSORY=8, OUI=16, SSID=32, IMPORTED=64, MALFORMED=128, SERIAL_PATTERN=256, FINGERPRINT=512 };
struct Match { uint32_t fingerprint=0; DetectionType type=DetectionType::UNKNOWN; Confidence conf=Confidence::LOW_CONF; uint16_t bits=0, rule=0; const char* label="Unknown"; };
struct Stats {
    bool active=false, raw=false; Profile profile=Profile::BALANCED;
    uint32_t session=0, start=0, elapsed=0, duration=300000, observed=0, saved=0, dropped=0, errors=0, bytes=0;
    uint32_t types[19][3]{}; // successfully saved observations by type/confidence; excludes annotations
    // DEAUTH frame records are evidence, not individual alerts. A coherent
    // burst is counted only when the main per-source detector crosses its
    // threshold. These remain session-level counters; RAW records contain
    // the per-frame addresses needed for field validation.
    uint32_t deauthFrames=0, deauthBursts=0, deauthMultiTargetBursts=0;
    uint32_t deauthProtected=0, deauthUnprotected=0, deauthReasonKnown=0;
    uint32_t bleMs=0, wifiMs=0, channelMs[14]{}; uint16_t channels=0; uint32_t catalog=BUILTIN_VERSION;
};
struct Record {
    uint32_t locationKey=0;
    uint32_t id=0, at=0, reference=0; uint8_t radio=0, mac[6]{}, addressType=0, channel=0;
    int8_t rssi=0; uint16_t original=0; uint8_t length=0; uint8_t payload[PAYLOAD_CAP]{};
    Match match; Verdict verdict=Verdict::UNREVIEWED; char note[49]{};
};
// Storage hooks execute only on the main task; no SD calls in radio callbacks.
using Sink = bool (*)(const char* json, const char* csv, bool reset);
void setSink(Sink sink);
using ReportSink = bool (*)(const Stats&);
void setReportSink(ReportSink);
bool formatReport(const Stats&,char*,size_t);
bool storageReport(const Stats&);
bool start(Profile profile, bool raw, uint32_t duration, uint32_t now, uint32_t randomSession, bool cardReady);
void stop(const char* reason="Stopped");
void discard();
Match matchManagement(const uint8_t* frame, size_t length);
void tick(uint32_t now);
bool settled(); // stopped, queue drained, summary written or an error reported
bool active(); Profile profile(); Stats stats(); const char* status();
void coverage(uint32_t deltaMs, bool bleListening, bool wifiListening, uint8_t channel);
void observe(uint8_t radio, const uint8_t* mac, uint8_t addressType, int8_t rssi, uint8_t channel,
             const uint8_t* payload, size_t length, uint32_t now, const Match& match=Match{});
// Called from the normal processing task after per-source burst analysis.
// It performs no storage I/O and deliberately records no address.
void noteDeauthBurst(uint8_t distinctTargets);
bool select(uint8_t index, Record& out);
bool recent(uint8_t index, Record& out); // newest first, eight records retained
bool annotate(uint32_t recordId, Verdict verdict, const char* note, uint32_t now);
Match matchBle(const uint8_t* payload, size_t size);
Match matchWifi(const uint8_t* mac, const char* ssid);
bool importPack(const char* data, size_t length); // bounded, transactional, experimental only
bool rollback(); uint32_t catalogVersion();
bool encode(const Record& record, const Stats& session, char* json, size_t jsonCap, char* csv, size_t csvCap);
bool storageImport(); // hardware adapter: /dnsp-signatures.txt, no network or code execution
void storageWipe();
const char* profileName(Profile p);
const char* verdictName(Verdict v);
}
