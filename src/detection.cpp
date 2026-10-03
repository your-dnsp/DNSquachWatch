#include "simulation.h"
// SquachWatch-CYD — DetectionEngine implementation
#include "detection.h"
static portMUX_TYPE s_sdMux = portMUX_INITIALIZER_UNLOCKED;
#include "location_label.h"
#include "ble_advert_fields.h"
#include "research.h"
#include "field_tools.h"
#include "drone_watch.h"
#include "detection_record.h"
#include "serial_flush.h"
#include "signatures.h"
#include "settings.h"
#include "blackbox.h"
#include "bingo.h"
#include "dex.h"
#include "regulars.h"
#include "clock.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_wifi_netif.h>
#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEScan.h>
#include "scan_profile.h"
// The host task's own event queue -- see the scan-result flush.
#if defined(CONFIG_NIMBLE_CPP_IDF)
#include "nimble/nimble_port.h"
#else
#include "nimble/porting/nimble/include/nimble/nimble_port.h"
#endif
#if SQUACH_MESH
#include "squachmesh.h"
#include "squachy.h"
#include "settings.h"
#endif
#include <esp_bt.h>
// esp_gap_bt_api.h was here and is not any more. It is a Bluetooth Classic
// header and does not exist on BLE-only silicon -- on the ESP32-C5 the build
// dies at "fatal error: esp_gap_bt_api.h: No such file or directory" before
// anything in this file compiles. Nothing here ever used a symbol from it:
// the only Classic reference in the file is esp_bt_controller_get_status()
// below, and that is declared in esp_bt.h, included above.
//
// Removing it rather than guarding it is deliberate. The obvious guard,
// #if SOC_BT_CLASSIC_SUPPORTED, is WRONG: that macro is in neither
// toolchain's soc_caps.h -- not IDF 4.4's for the ESP32, nor IDF 5.5's for
// the C5 -- so it evaluates to 0 everywhere. On the C5 that happens to give
// the right answer; on the ESP32 it silently compiled the status check out
// and shrank [env:cyd]'s .text by 8 bytes. A guard that is right by accident
// on one target and wrong on another is worse than no guard.
#include <esp_heap_caps.h>
#include <string.h>
#include <SD.h>

// Borrow fields from the advertisement for this callback only. NimBLE's
// string getters copy into heap allocations, even though none of these
// consumers needs to own the bytes. Bounds are checked before reading a field.
static AdvertField advertField(const NimBLEAdvertisedDevice* adv, uint8_t type, unsigned index=0) {
    const auto& payload=adv->getPayload();
    return findAdvertField(payload.data(),payload.size(),type,index);
}
static void advertName(const NimBLEAdvertisedDevice* adv, char* out, size_t capacity) {
    if (!capacity) return;
    const auto field=advertField(adv,0x09);
    const size_t n=field.size()<capacity-1 ? field.size() : capacity-1;
    if (n) memcpy(out,field.data(),n);
    out[n]=0;
}

// -------- global engine instance (referenced by callbacks) --------
static DetectionEngine* g_engine = nullptr;

// -------- manual raw scanner state (see startRawBleScan/startRawWifiScan) --------
// NONE = normal continuous signature-matched scanning (the default).
// Only one of these is ever active at a time -- see stopRawScan().
// REST is the watch's radio duty cycle: WiFi stopped, BLE scanning stopped
// or left running (s_restBle). Detections that do arrive are handled as
// usual, which is what sets it apart from UPDATE.
enum class RawScanMode : uint8_t { NONE, BLE, WIFI, UPDATE, REST };
static bool g_restBle = false;   // REST: is the BLE scan resting too?
static RawScanMode g_rawMode        = RawScanMode::NONE;
static uint32_t    g_rawBleStartMs  = 0;
// How long a raw BLE sweep stays open before the UI is told it's
// "done" -- a deliberately longer, focused dwell than the continuous
// scan ever gives any one moment, which is the actual "more thorough"
// part; devices keep updating in _rawBle past this point too (nothing
// stops capturing), it's purely a UI cue for when to stop showing
// "SCANNING..." and reveal the list.
static const uint32_t RAW_BLE_SCAN_MS = 8000;

// -------- helpers --------

static void formatMac(char* dst, size_t dstSize, const uint8_t* mac) {
    snprintf(dst, dstSize, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// scan->start(0, ...) starts an indefinite scan (0 = "forever" per
// NimBLEScan::start()'s own implementation); its completion callback
// never fires for a forever-scan, which is why an early version of this
// file never detected anything. Live detection comes from the scan
// callbacks set with setScanCallbacks(): onDiscovered() for every advert
// as it arrives, onResult() once the library considers a device's data
// complete (at once in a passive scan; after the reply or the reply
// timeout in an active one).
static uint32_t s_advertsDropped = 0;   // adverts refused for want of heap; reported with the flush
// Set on the loop task by scanFlushTick from one heap walk a frame, and read
// here on the host task: the walk itself takes the heap lock and a few
// hundred microseconds, and at 130 adverts a second that is not a thing to
// do per advert on the task that also has to receive them.
static volatile bool s_heapLow = false;
// Called on the host task BEFORE the pinned library constructs/updates an
// advertiser. This cannot guarantee success against concurrent allocations,
// but avoids knowingly allocating on a heap consisting of tiny fragments.
extern "C" void dnsp_ble_receive_drop() { ++s_advertsDropped; }
extern "C" bool dnsp_ble_receive_room() {
    if (heap_caps_get_free_size(MALLOC_CAP_8BIT)<6144 ||
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)<2048) {
        ++s_advertsDropped;
        return false;
    }
    return true;
}

static BootHeap s_bootHeap = { 0, 0, 0, 0 };
BootHeap bootHeap()       { return s_bootHeap; }
uint32_t advertsDropped() { return s_advertsDropped; }

static volatile uint32_t s_advRaw = 0;   // every advert the radio handed over, seatbelt or not
// ...and by kind: 0 ADV_IND (connectable, scannable), 1 DIRECT, 2 SCAN_IND
// (scannable only), 3 NONCONN (neither), 4 anything else. Only kinds 0 and 2
// make an active scanner wait for a reply, so only they can pile its list up.
static volatile uint32_t s_advKind[5] = { 0, 0, 0, 0, 0 };
const volatile uint32_t* advertKinds() { return s_advKind; }
static volatile uint32_t s_wifiRaw = 0;  // every frame the sniffer was handed
static volatile uint16_t s_chanFrames[14] = {0};
static uint16_t s_chanRate[14] = {0}; // smoothed frames/second, multiplied by 16
uint32_t wifiFramesSeen() { return s_wifiRaw; }
uint32_t advertsSeen()    { return s_advRaw; }

// The RADIO console command: what the radios are doing right now, so a boot
// that hears and a boot that does not can be compared side by side. With
// "SCAN" it also runs the driver's own WiFi scan (sniffer paused for it).
char g_bootRadioLine[192] = "";
void radioReport(bool withScan) {
    if (g_bootRadioLine[0]) Serial.printf("[radio] boot: %s\n", g_bootRadioLine);
    wifi_mode_t mode = WIFI_MODE_NULL;
    const esp_err_t me = esp_wifi_get_mode(&mode);
    bool promisc = false;
    esp_wifi_get_promiscuous(&promisc);
    uint8_t ch = 0; wifi_second_chan_t ch2 = WIFI_SECOND_CHAN_NONE;
    esp_wifi_get_channel(&ch, &ch2);
    int8_t txp = 0;
    esp_wifi_get_max_tx_power(&txp);
    wifi_country_t cc = {};
    esp_wifi_get_country(&cc);
    Serial.printf("[radio] wifi: mode %d (err %d), sniffer %s, channel %u, tx max %d, country %.2s %u-%u, frames %lu\n",
                  (int)mode, (int)me, promisc ? "on" : "off", (unsigned)ch, (int)txp, cc.cc,
                  (unsigned)cc.schan, (unsigned)(cc.schan + cc.nchan - 1), (unsigned long)s_wifiRaw);
    NimBLEScan* sc = NimBLEDevice::getScan();
    Serial.printf("[radio] ble: init %d, scanning %d, adverts %lu, raw mode %d, chip %.1f C, up %lu s\n",
                  (int)NimBLEDevice::isInitialized(), sc ? (int)sc->isScanning() : -1,
                  (unsigned long)s_advRaw, (int)g_rawMode, temperatureRead(), (unsigned long)(millis() / 1000));
    if (!withScan) return;
    esp_wifi_set_promiscuous(false);
    wifi_scan_config_t cfg = {};
    cfg.show_hidden = true;
    cfg.scan_type = WIFI_SCAN_TYPE_PASSIVE;
    cfg.scan_time.passive = 150;
    const uint32_t t0 = millis();
    const esp_err_t se = esp_wifi_scan_start(&cfg, true);
    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    int8_t best = -127;
    if (n) {
        uint16_t k = n > 20 ? 20 : n;
        wifi_ap_record_t recs[20];
        esp_wifi_scan_get_ap_records(&k, recs);
        for (uint16_t i = 0; i < k; i++) if (recs[i].rssi > best) best = recs[i].rssi;
    } else {
        esp_wifi_clear_ap_list();
    }
    Serial.printf("[radio] driver scan: err %d, %u network(s), strongest %d dBm, %lu ms\n",
                  (int)se, (unsigned)n, n ? (int)best : 0, (unsigned long)(millis() - t0));
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(ch ? ch : 1, WIFI_SECOND_CHAN_NONE);
}
static volatile uint8_t s_windowReq = 0;   // a WINDOW command waiting for the next restart
static bool             s_windowPending = false;
void setScanWindow(uint8_t w) { if (w >= 1 && w <= 100) { s_windowReq = w; s_windowPending = true; } }
static uint8_t s_baseWindow = 75;
static bool s_scanBoosted = false;
void setScanWindowBase(uint8_t w) {
    if (w < 1 || w > 100 || w == s_baseWindow) return;
    s_baseWindow = w;
    if (!s_scanBoosted) setScanWindow(w);
}
void setScanBoost(bool on) {
    s_scanBoosted = on;
    setScanWindow(on ? 99 : s_baseWindow);
}
// BENCH: INTERVAL N (ms), with the window in the same message; see scanFlushOnHost.
static volatile uint16_t s_intervalReq = 0;
void setScanInterval(uint16_t ms, uint8_t w) {
    if (ms < 20 || ms > 1000 || w < 1 || w > ms) return;
    s_intervalReq = ms; s_windowReq = w; s_windowPending = true;
}
static volatile uint8_t s_scanPin = 0;   // 0 auto, 1 active, 2 passive -- the bench's say
void setScanPin(uint8_t pin) { s_scanPin = pin > 2 ? 0 : pin; }

class BleScanCallbacks : public NimBLEScanCallbacks {
    // First sight of every advert, before any reply. The counts live here
    // and not in onResult: in an active scan a device that never answers
    // is not handed to onResult until the wait times out, and the advert
    // rate the passive switch runs on has to see exactly those devices --
    // they are the ones that fill the list. Measured with the fake flood:
    // counted at onResult, 200 unanswering adverts a second read as a
    // quiet room right up to the crash.
    void onDiscovered(const NimBLEAdvertisedDevice* adv) override {
        s_advRaw++;
        const uint8_t t = adv->getAdvType();
        s_advKind[t == BLE_HCI_ADV_TYPE_ADV_IND ? 0 : t == BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_HD ? 1 :
                  t == BLE_HCI_ADV_TYPE_ADV_SCAN_IND ? 2 : t == BLE_HCI_ADV_TYPE_ADV_NONCONN_IND ? 3 : 4]++;
#if SQUACH_MESH
        // Counted here, once per advert, whatever handle() goes on to do
        // with it. The measurement is of what the RADIO heard, not of what
        // the signature tables liked, and not of how many times the library
        // hands the same advert over (see below: up to twice).
        MeshProbe::noteAdvert();
#endif
        // Detection at first sight, for the one case the library holds back:
        // an active scan and a scannable advert. The library waits for that
        // device's reply before calling onResult, and a device that never
        // answers but keeps advertising restarts that wait every time, so it
        // would never be reported at all -- and a scan restart deletes it
        // unreported. Everything else (passive, or an advert nobody asks) is
        // handed to onResult at once and is handled there. When a reply or
        // the reply timeout does come, onResult runs the same handling again
        // with whatever the reply added (a name, a squad message); the log
        // keeps the entry it already has and takes the name from the reply
        // (see postBle).
        if (!scanPassiveNow() && adv->isLegacyAdvertisement() && adv->isScannable()) handle(adv);
    }
    void onResult(const NimBLEAdvertisedDevice* adv) override { handle(adv); }
    void handle(const NimBLEAdvertisedDevice* adv) {
        // Retain the low-heap guard for downstream/library work. Name and
        // manufacturer parsing below now borrows the payload without allocating;
        // NimBLE's own receive allocations still happen before this callback.
        if (s_heapLow) {
            s_advertsDropped++;
            return;
        }
        // 2.x hands back a reference to the device's own address, so the
        // pointer is good for the whole of this call.
        // NimBLE exposes its internal little-endian address bytes. Mesh peers
        // already use that internal order on-air, but every user-facing MAC
        // and every ordinary detector must use the conventional printed order.
        const uint8_t* nimbleMac = adv->getAddress().getBase()->val;
#if SQUACH_MESH
        // A peer is handled here and RETURNS, so it never reaches the
        // signature tables and can never become a Detection. Getting that
        // wrong would have two SquachWatches alarming at each other -- the
        // exact failure the HACKER bucket was shaped to avoid.
        // Every manufacturer-data block, not just the first. A peer sending a
        // message carries TWO -- its advert's, then its scan response's (see
        // include/meshmsg.h for why the message rides there) -- in that order,
        // which is what lets the name from the first travel with the second.
        // v1.5.23 and earlier only ever read the first, and that is exactly
        // what keeps them seeing a peer that is in the middle of a message.
        if (adv->haveManufacturerData()) {
            bool ours = false;
            const uint8_t mdN = adv->getManufacturerDataCount();
            for (uint8_t i = 0; i < mdN; i++) {
                const auto md = advertField(adv, 0xff, i);
                if (Mesh::onManufacturerData((const uint8_t*)md.data(), md.size(), nimbleMac, millis())) ours = true;
            }
            // A SquachWatch is not a detection, but it can be a hunt target:
            // the SQUAD screen's HUNT aims the gauge at one. Its advert feeds
            // the watch and hunt slots and then stops here, as before.
            if (ours) {
                if (g_engine) {
                    const int8_t r = (int8_t)adv->getRSSI();
                    g_engine->checkWatchBle(nimbleMac, r);
                    g_engine->checkHuntBle(nimbleMac, r);
                }
                return;
            }
        }
#endif
        uint8_t printedMac[6];
        for (uint8_t i = 0; i < 6; i++) printedMac[i] = nimbleMac[5 - i];
        const uint8_t* mac = printedMac;
        const bool ridPayloadValid=Field::observeBle(mac, adv->getPayload().data(), adv->getPayload().size(), millis());
        DroneWatch::observe(false, mac, adv->getPayload().data(), adv->getPayload().size(), adv->getRSSI(), 0, millis());
        const auto researchMatch = Research::matchBle(adv->getPayload().data(), adv->getPayload().size());
        Research::observe(0, mac, adv->getAddress().getType(), (int8_t)adv->getRSSI(), 0,
                          adv->getPayload().data(), adv->getPayload().size(), millis(), researchMatch);
        if (!g_engine) return;
        // Checked regardless of raw-scan mode -- a watched/hunted
        // target still fires even if it's not a known signature and
        // even while the raw-scan screen happens to be open. The two
        // are independent slots (see detection.h), so both are always
        // checked -- either, both, or neither can match a given frame.
        int8_t rssi = (int8_t)adv->getRSSI();
        g_engine->checkWatchBle(mac, rssi);
        g_engine->checkHuntBle(mac, rssi);
        // The radio is dedicated to a WiFi sweep or a firmware update right now.
        if (g_rawMode == RawScanMode::WIFI || g_rawMode == RawScanMode::UPDATE) return;
        if (g_rawMode == RawScanMode::BLE) {
            RawBleResult r;
            memset(&r, 0, sizeof(r));
            memcpy(r.mac, mac, 6);
            r.rssi = adv->getRSSI();
            advertName(adv, r.name, sizeof r.name);
            g_engine->postRawBle(r);
            return;
        }
        Detection det;
        memset(&det, 0, sizeof(det));
        memcpy(det.mac, mac, 6);
        det.addressRole=AddressRole::BLE_ADVERTISER;
        det.rssi   = adv->getRSSI();
        det.channel= 0;
        det.firstSeen = det.lastSeen = millis();
        det.hits   = 1;
        det.active = true;
        advertName(adv, det.name, sizeof det.name);
        // The matched row's own label, for the types that cover several
        // devices -- see where the vendor is written, below.
        const char* label = nullptr;
        // Manufacturer data
        if (adv->haveManufacturerData()) {
            const auto mfg = advertField(adv, 0xff);
            if (mfg.size() >= 2) {
                uint16_t mfgId = (uint8_t)mfg[0] | ((uint8_t)mfg[1] << 8);
                det.type = lookupMfgId(mfgId);
                label    = mfgIdName(mfgId);
                det.evidence = MatchEvidence::BLE_COMPANY;
                det.signature = mfgId;
                // Apple's company ID alone is every Apple device, so it
                // still has to be confirmed as a tag. That check now
                // runs against the RAW advert rather than this parsed
                // field -- see isAirTagPayload().
                if (det.type == DetectionType::AIRTAG) {
                    if (!isAirTagPayload(adv->getPayload().data(),
                                         (uint8_t)adv->getPayload().size())) {
                        det.type = DetectionType::UNKNOWN;
                        label    = nullptr;
                        // Apple's company ID is also how an iBeacon announces
                        // itself, so this is where they used to die: not an
                        // AirTag, therefore nothing, therefore dropped. They
                        // are probably the most numerous tracking transmitter
                        // most people walk past in a day.
                        const uint8_t* b = (const uint8_t*)mfg.data();
                        if (isIBeacon(b, (uint8_t)mfg.size())) {
                            det.type = DetectionType::IBEACON;
                            det.evidence = MatchEvidence::IBEACON;
                            // Major and minor are BIG endian here, unlike the
                            // company ID two bytes earlier -- Apple's format
                            // is network order inside the block and Bluetooth
                            // order outside it.
                            const unsigned major = (unsigned)((b[20] << 8) | b[21]);
                            const unsigned minor = (unsigned)((b[22] << 8) | b[23]);
                            // Six hex of the proximity UUID plus major.minor.
                            // The UUID is the DEPLOYMENT -- every beacon a
                            // chain owns shares it -- so two sightings with
                            // the same first half are the same operator in two
                            // places, which is the part worth seeing. Six and
                            // not eight so the unit number cannot be truncated
                            // off the end of a 20-byte field.
                            snprintf(det.name, sizeof(det.name), "%02X%02X%02X %u.%u",
                                     b[4], b[5], b[6], major, minor);
                        }
                    }
                }
            }
        }
        // Raw-advert fallback. The block above only runs when NimBLE
        // parsed a manufacturer-data field and put Apple's company ID
        // first; the reference implementation this came from does not
        // depend on either, it just scans the bytes. An advert that
        // carries the Find My structure behind another AD structure, or
        // in a scan response, reaches the detector only through here.
        if (det.type == DetectionType::UNKNOWN &&
            isAirTagPayload(adv->getPayload().data(), (uint8_t)adv->getPayload().size())) {
            det.type = DetectionType::AIRTAG;
        }

        // Service UUIDs
        if (det.type == DetectionType::UNKNOWN && adv->haveServiceUUID()) {
            for (int j = 0; j < adv->getServiceUUIDCount(); j++) {
                NimBLEUUID u = adv->getServiceUUID(j);
                // 16-bit UUID match: avoid touching ble_uuid_t's
                // internals (the struct layout varies between
                // NimBLE-Arduino versions). The equals() method is
                // a stable API and compares the logical 16-bit value.
                if (u.bitSize() == 16) {
                    static const uint16_t kKnown16[] = {
                        0x1101,  // SPP — skimmer
                        0xFEED,  // Tile tracker
                        0xFEEC,  // Tile tracker (second SIG-assigned UUID)
                        0xFD5F,  // Ray-Ban Meta glasses
                        0x3100, 0x3200, 0x3300, 0x3400, 0x3500,  // Raven
                        0xFFFA,  // OpenDroneID
                        0xFD5A,  // Samsung SmartTag
                        0xFEAA,  // Google Find My Device Network (Eddystone)
                        0x3081, 0x3082, 0x3083,  // Flipper Zero, one per case colour
                    };
                    for (uint16_t k : kKnown16) {
                        if (u.equals(NimBLEUUID((uint16_t)k))) {
                            det.type = lookupUuid(k);
                            label    = uuidName(k);
                            det.evidence = MatchEvidence::BLE_SERVICE;
                            det.signature = k;
                            break;
                        }
                    }
                    if (det.type != DetectionType::UNKNOWN) break;
                }
            }
        }
        // Name fallback
        bool matchedByName = false;
        if (det.type == DetectionType::UNKNOWN && det.name[0]) {
            det.type = lookupBtName(det.name);
            label    = nullptr;          // the name itself identifies it
            matchedByName = (det.type != DetectionType::UNKNOWN);
            if (matchedByName) det.evidence = MatchEvidence::BLE_NAME;
        }
        DroneWatch::applyDecodedBle(det, ridPayloadValid);
        if (det.type == DetectionType::UNKNOWN && researchMatch.type == DetectionType::UNKNOWN) return;
        if (det.type == DetectionType::AIRTAG) det.evidence = MatchEvidence::FIND_MY;
        // BLE matches on service UUIDs, company IDs and device names --
        // none of those tables has a per-row grade, so the type's own
        // grade stands. Only the OUI table needed splitting.
        det.conf = confidenceFor(det.type);
        // ...except HACKER, which is the one type deliberately holding
        // signatures of very different strength. Reaching here on anything
        // but the name means one of the exact ones matched: a service UUID
        // that exists on no other product, or Flipper's own SIG company ID.
        // Arriving on the name alone means somebody's BLE device is called
        // "Flipper", which is a string, not a signature.
        if (det.type == DetectionType::HACKER) {
            det.conf = matchedByName ? Confidence::MED_CONF : Confidence::HIGH_CONF;
        }
        if (researchMatch.type != DetectionType::UNKNOWN) {
            det.type = researchMatch.type; det.conf = researchMatch.conf; label = researchMatch.label;
            det.evidence = MatchEvidence::RESEARCH_COMPOSITE; det.signature = researchMatch.rule; det.evidenceBits = researchMatch.bits;
        }
        DroneWatch::applyDecodedBle(det, ridPayloadValid);
        // A Remote ID advert carries far more than the fact that it exists.
        // Decode it before the entry is posted so the log row can be named
        // after the actual aircraft rather than after a service UUID.
        if (det.type == DetectionType::DRONE) {
            det.conf=ridPayloadValid?Confidence::MED_CONF:Confidence::LOW_CONF;
            g_engine->mergeRemoteId(mac, adv->getPayload().data(),
                                    (uint8_t)adv->getPayload().size());
            const RemoteId::Info& rid = g_engine->remoteId();
            if (rid.haveBasic && rid.serial[0]) {
                strncpy(det.name, rid.serial, sizeof(det.name) - 1);
                det.name[sizeof(det.name) - 1] = '\0';
            }
        }
        // Set vendor label based on the matched table entry.
        if (det.type == DetectionType::AIRTAG) {
            det.vendor = "Apple";
        } else if (det.type == DetectionType::DRONE) {
            det.vendor = "DroneID";
        } else if (det.type == DetectionType::META) {
            // Which row matched, because META is four devices: Ray-Ban
            // Meta's own UUID, any Meta radio, Luxottica, Snap Spectacles.
            det.vendor = label ? label : "Meta";
        } else if (det.type == DetectionType::RAVEN) {
            det.vendor = "Raven";
        } else if (det.type == DetectionType::FLOCK) {
            det.vendor = "Flock-BLE";
        } else if (det.type == DetectionType::SAMSUNG_TAG) {
            det.vendor = "Samsung";
        } else if (det.type == DetectionType::GOOGLE_TAG) {
            det.vendor = "Google";
        } else if (det.type == DetectionType::TILE) {
            det.vendor = "Tile";
        } else if (det.type == DetectionType::IBEACON) {
            det.vendor = "iBeacon";
        } else if (det.type == DetectionType::HACKER) {
            // Everything that reaches HACKER over BLE is a Flipper: the
            // three service UUIDs, the company ID and the name prefix are
            // all theirs. The Pineapple and the deauther arrive over WiFi
            // and are labelled in processWiFiQ().
            det.vendor = "Flipper";
        } else if (det.type == DetectionType::SKIMMER && label) {
            // The serial-port UUID's row. A skimmer matched on its NAME has
            // no label and stays "BLE": its page is found by the name.
            det.vendor = label;
        } else {
            det.vendor = "BLE";
        }
        g_engine->postBle(det);
    }
};
static BleScanCallbacks g_bleScanCallbacks;

// -------- DetectionEngine --------

void DetectionEngine::saveLifetimeByType() {
    _prefs.putBytes("typetot", _lifetimeByType, sizeof(_lifetimeByType));
}

void DetectionEngine::resetLifetime() {
    _lifetimeTotal  = 0;
    _lifetimeDirty  = false;
    _prefs.putUInt("total", 0);
    for (uint8_t i = 0; i < (uint8_t)DetectionType::COUNT; i++) {
        _lifetimeByType[i] = 0;
    }
    saveLifetimeByType();
}

bool DetectionEngine::init() {
    if (g_engine) return true;
    g_engine = this;

    // The SD card is mounted LAST, after both radios -- see the end of this
    // function. It used to be first, and on a board with a card in the slot
    // the mount took the heap Bluetooth's controller needs a moment later:
    // NimBLEDevice::init() hit ESP_ERR_NO_MEM inside an ESP_ERROR_CHECK,
    // aborted, and the board rebooted every three seconds, forever. That was
    // "the SD boot loop", filed for months as a display-driver problem
    // because the boards it showed on happened to be the ones with cards in.

    // Lifetime detection count survives reboots — Squachy references it
    // for milestone quips. Live _typeCounts above deliberately don't
    // persist (they decay when a detection goes stale), so this is
    // tracked separately.
    _prefs.begin("squachwatch", false);
    _lifetimeTotal = _prefs.getUInt("total", 0);
    // A short read means the key predates this field or the type list has
    // grown since it was written; take what is there and leave the rest at
    // zero rather than discarding counts someone has spent months earning.
    {
        size_t have = _prefs.getBytesLength("typetot");
        if (have > sizeof(_lifetimeByType)) have = sizeof(_lifetimeByType);
        if (have >= sizeof(uint32_t)) _prefs.getBytes("typetot", _lifetimeByType, have);
    }

    // 2. WiFi promiscuous mode for OUI/SSID detection
    //
    // Each radio start is announced -- and flushed, so the line is out before
    // the step that might brown the board out -- because a brownout leaves no
    // crash dump and the reset reason alone does not say which of the two it
    // was. A board that boot-loops prints the last one it reached.
    static const bool SLIM_WIFI = true;    // A/B on the bench: 45 frames/40 s slim, 20/45 s stock
    Serial.println("[boot] starting WiFi");
    serialFlush();
    Serial.printf("[boot] heap before WiFi: %lu free, %lu largest\n", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT), (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(50);
    // The driver again, on a sniffer's budget. The Arduino core starts WiFi
    // sized for a laptop-style connection: four DMA receive buffers, a
    // cache of transmit buffers, packet aggregation both ways. A board that
    // listens and only ever transmits for an update needs the minimum of
    // each, and the difference is heap nothing else can reach. The core's
    // network interface stays; it is re-attached to the new driver so an
    // update over WiFi still gets an address.
    if (SLIM_WIFI) {
        esp_wifi_stop();
        esp_wifi_deinit();
        // The driver teardown finishes asynchronously on some targets. A
        // short gap avoids rebuilding it in the same RTOS tick and starting
        // with the receiver silently disabled.
        delay(10);
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.static_rx_buf_num  = 2;    // the DMA landing zone; 2 is the floor
        cfg.dynamic_rx_buf_num = 16;
        cfg.static_tx_buf_num  = 0;
        cfg.dynamic_tx_buf_num = 8;
        cfg.tx_buf_type        = 1;
        cfg.cache_tx_buf_num   = 1;    // cannot be zero with dynamic TX
        cfg.ampdu_rx_enable    = 0;
        cfg.ampdu_tx_enable    = 0;
        cfg.amsdu_tx_enable    = 0;
        cfg.csi_enable         = 0;
        cfg.mgmt_sbuf_num      = 6;    // the floor
        cfg.nvs_enable         = 0;
        const esp_err_t rc = esp_wifi_init(&cfg);
        if (rc == ESP_OK) {
            esp_netif_t* sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (sta) esp_netif_attach_wifi_station(sta);
            esp_wifi_set_mode(WIFI_MODE_STA);
            esp_wifi_start();
        } else {
            Serial.printf("[boot] slim WiFi init failed (%d), back to the core's\n", (int)rc);
            WiFi.mode(WIFI_OFF);
            WiFi.mode(WIFI_STA);
        }
    }
    Serial.printf("[boot] heap with WiFi started: %lu free, %lu largest\n", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT), (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    esp_wifi_set_promiscuous(true);
    wifi_promiscuous_filter_t filter;
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb([](void* buf, wifi_promiscuous_pkt_type_t) {
        s_wifiRaw++;
        if (!g_engine) return;
        const wifi_promiscuous_pkt_t* pkt = (const wifi_promiscuous_pkt_t*)buf;
        if (pkt->rx_ctrl.rx_state != 0 || pkt->rx_ctrl.sig_len < 28) return;
        const uint8_t heardChannel = pkt->rx_ctrl.channel;
        if (heardChannel >= 1 && heardChannel <= 13 && s_chanFrames[heardChannel] != 0xFFFF)
            ++s_chanFrames[heardChannel];
        const size_t frameLength = pkt->rx_ctrl.sig_len - 4; // ESP-IDF length includes FCS
        // 802.11 frame header: bytes 0..23 contain frame control, duration,
        // addr1 (DA, offset 4), addr2 (SA, offset 10), addr3 (BSSID, offset 16)
        const uint8_t* frame = pkt->payload;
        uint8_t fc0 = frame[0];
        uint8_t type  = (fc0 & 0x0C) >> 2;
        uint8_t subtype = (fc0 & 0xF0) >> 4;
        if (type == 0) DroneWatch::observe(true, frame+10, frame, frameLength, pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, millis());
        // Research stores management headers/IE prefixes only, never data-frame contents.
        if (type == 0 && Field::observeWifi(frame,frameLength,millis())) {
            Detection d{};memcpy(d.mac,frame+10,6);d.rssi=pkt->rx_ctrl.rssi;d.channel=pkt->rx_ctrl.channel;
            d.type=DetectionType::DRONE;d.conf=Confidence::MED_CONF;d.vendor="WiFi Remote ID";
            d.addressRole=AddressRole::TRANSMITTER;d.evidence=MatchEvidence::WIFI_REMOTE_ID;d.firstSeen=d.lastSeen=millis();d.hits=1;d.active=true;
            g_engine->postBle(d);
        }
        if (type == 0) {
            const auto match = Research::matchManagement(frame, frameLength);
            Research::observe(1, frame + 10, (frame[10] & 2) ? 1 : 0,
                pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, frame, frameLength, millis(), match);
            if ((match.bits & Research::FINGERPRINT) && match.type != DetectionType::UNKNOWN) {
                Detection d{};memcpy(d.mac,frame+10,6);d.rssi=pkt->rx_ctrl.rssi;d.channel=pkt->rx_ctrl.channel;
                d.type=match.type;d.conf=Confidence::LOW_CONF;d.vendor="IE-class?";d.signature=match.rule; d.evidenceBits=match.bits;
                d.addressRole=AddressRole::TRANSMITTER;d.evidence=MatchEvidence::RESEARCH_COMPOSITE;d.firstSeen=d.lastSeen=millis();d.hits=1;d.active=true;
                g_engine->postBle(d); // bounded cross-task observation queue; channel still identifies WiFi
            }
        }
        // Management frame probe request: type=0, subtype=4
        if (type == 0 && subtype == 4) {
            // addr2 (transmitter) is at offset 10
            g_engine->postWiFi(frame + 10, pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel);
        } else if (type == 2) {
            // Data frame: addr1 (DA) and addr2 (SA) both interesting
            // addr1 is the recipient, whose signal was not measured. Only classify transmitter.
            g_engine->postWiFi(frame + 10, pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel);
        } else if (type == 0 && subtype == 8) {
            // Beacon: fixed params (timestamp+interval+capability) run
            // 12 bytes after the 24-byte header, then the SSID is the
            // first information element — tag 0x00, 1-byte length,
            // then up to 32 bytes of SSID (unescaped, not
            // null-terminated in the frame itself).
            char ssid[33] = {0};
            uint32_t sigLen = frameLength; // exclude FCS from all information-element bounds
            if (sigLen > 37) {
                const uint8_t* ie = frame + 36;
                if (ie[0] == 0x00) {
                    uint8_t ssidLen = ie[1];
                    if (ssidLen > 32) ssidLen = 32;
                    if (36 + 2 + ssidLen <= sigLen) {
                        memcpy(ssid, ie + 2, ssidLen);
                        ssid[ssidLen] = 0;
                    }
                }
            }
            // Capability info is the last 2 bytes of the 12-byte fixed
            // parameter block following the 24-byte header, so offset
            // 34. Bit 4 (0x10) is Privacy: set means this BSS requires
            // encryption. That one bit is what separates a mesh node
            // from an evil twin -- see noteApBeacon().
            bool enc = (sigLen > 35) && ((frame[34] & 0x10) != 0);
            // A pwnagotchi's beacon is checked before it is treated as an
            // ordinary AP, and posts addr2 rather than the BSSID: this is a
            // device announcing itself to its own kind, not an access point
            // offering a network, and its SSID is throwaway. Its name goes
            // where the SSID would have.
            char pwnName[33];
            if (pwnagotchiName(frame, sigLen, pwnName, sizeof(pwnName))) {
                g_engine->postWiFi(frame + 10, pkt->rx_ctrl.rssi,
                                   pkt->rx_ctrl.channel, pwnName, false, true);
            } else {
                g_engine->postWiFi(frame + 16, pkt->rx_ctrl.rssi, pkt->rx_ctrl.channel, ssid, enc, false, false, AddressRole::BSSID);
            }
        } else if (type == 0 && subtype == 12) {
            // Copy only fixed-header evidence here. Address 2 is the claimed
            // transmitter, not proof of physical identity: management-frame
            // addresses can be spoofed. One frame is ordinary WiFi traffic;
            // bounded per-source analysis happens later in loop().
            DeauthFrameEvidence evidence{};
            if (parseDeauthFrame(frame, frameLength, pkt->rx_ctrl.rssi,
                                 pkt->rx_ctrl.channel, evidence))
                g_engine->postDeauth(evidence);
        }
    });

    // 3. NimBLE scan — onResult() fires live per-advertisement via
    // g_bleScanCallbacks (see above), not via a scan-complete callback
    // that would never fire on an indefinite (duration 0) scan.
    // A breath between the two radios' start-up bursts, so the supply is not
    // asked for both at once -- see the backlight note in main.cpp's setup().
    delay(150);
    s_bootHeap.wifiFree    = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    s_bootHeap.wifiLargest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    Serial.printf("[boot] heap with WiFi up: %lu free, %lu largest\n",
                  (unsigned long)s_bootHeap.wifiFree, (unsigned long)s_bootHeap.wifiLargest);
    Serial.println("[boot] starting Bluetooth");
    serialFlush();
    NimBLEDevice::init("");
    NimBLEScan* scan = NimBLEDevice::getScan();
    // Passive to start, whatever the room: the first second of an active
    // scan in a crowded building is what killed a user's board. Active
    // comes once the rate of adverts says the room can afford it -- see
    // scanModeTick() below.
    scan->setActiveScan(false);
    scan->setInterval(100);
    // The Bluetooth scan and the WiFi sniffer share one radio, and the scan
    // window is the share. Measured on the bench 2026-09-15, four minutes each
    // at the same spot: window 99 gave 44 WiFi frames and 2641 adverts, 75 gave
    // 866 frames and 2458 adverts, 50 gave 1851 frames and 1485 adverts. At 99
    // the sniffer was deaf -- one frame a second in a house with a router
    // beaconing ten times a second -- and 75 buys it twenty times that for a
    // dip in adverts inside run-to-run noise. 50 costs half the adverts.
    scan->setWindow(ScanProfile::bleShare());
    scan->setDuplicateFilter(false);
    // How long an active scan waits for a device's reply before reporting
    // it as it is and letting its record go. The library's default is the
    // longest advertising interval, 10.24 s, and at 200 unanswering devices
    // a second that is two thousand records -- the college-floor crash, and
    // the fake flood reproduces it in seconds. A reply that is coming
    // arrives inside the same advertising event, well under a millisecond,
    // so 200 ms loses none and bounds the list to a few dozen records.
    scan->setScanResponseTimeout(200);
    // wantDuplicates=true: every sighting of a device reaches the
    // callbacks, not just the first, so lastSeen/RSSI keep updating
    // (postBle()/expireStale() rely on that for the still-active log).
    scan->setScanCallbacks(&g_bleScanCallbacks, true);
    // Without this, NimBLE keeps its OWN permanent record of every
    // distinct BLE address it has ever seen since scan start (a
    // separate, unbounded structure from our own bounded 200-entry
    // _log) -- one new NimBLEAdvertisedDevice heap allocation per
    // never-before-seen address, held forever because this scan runs
    // indefinitely (duration 0) and never fires a scan-complete
    // callback to clear it. Confirmed on real hardware: a commute
    // exposes a steady stream of genuinely distinct devices (phones,
    // headsets, cars, AirTags), and that leak crashed the device after
    // enough of them. maxResults=0 is documented in NimBLEScan.cpp as
    // "none (callbacks only)" — it deletes each device immediately
    // after onResult() returns instead of retaining it, which is
    // exactly this project's usage (everything comes through the
    // per-advertisement callback; the retained-results list and the
    // scan-complete callback below are never read).
    scan->setMaxResults(0);
    scan->start(0, false, false);   // forever; not a restart
    s_bootHeap.bleFree    = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    s_bootHeap.bleLargest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    Serial.printf("[boot] heap with Bluetooth up: %lu free, %lu largest\n",
                  (unsigned long)s_bootHeap.bleFree, (unsigned long)s_bootHeap.bleLargest);

    // 4. BT Classic inquiry for skimmer names (best-effort, every 60 s)
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        // We don't try to *start* the controller here — NimBLE may have
        // taken it over. The v1.0 implementation is BLE-only for skimmers
        // (advertised name match). Documented in docs/DETECTIONS.md.
    }

    // 5. SD card (best-effort), now that the radios have what they need.
    _sd.begin();

    return true;
}

#if SQUACH_MESH
namespace MeshProbe {

// THE EXPERIMENT IS OVER, and this is what is left of it.
//
// It alternated advertising on and off in thirty-second arms to find out
// whether transmitting costs the scanner anything. Answer, measured on
// hardware over sixty-two arms: 68.7 adverts/sec with advertising off
// against 68.6 with it on at a 1500ms interval. Nothing.
//
// It had to be torn out rather than left running, because it advertised
// WITHOUT ASKING. Only Mesh::tick consulted Settings::meshEnabled(); the
// probe drove the same NimBLE advertising singleton straight past it, so a
// device told not to announce itself announced itself every other arm. On a
// tool whose whole premise is that it does not transmit unless asked, that
// is not a measurement artefact, it is the thing the setting exists to
// prevent.
//
// It also fought Mesh for the same singleton the rest of the time, switching
// advertising off for thirty seconds at a stretch while a peer was trying to
// find us.
//
// What stays is the counter, which costs one increment per advert and
// answers "is the radio actually hearing anything" -- worth having when a
// peer does not turn up and the question is whether the scanner is alive.
static uint32_t s_seen    = 0;
static uint32_t s_windowAt = 0;
static uint16_t s_rate     = 0;   // adverts/sec x10

void noteAdvert() { s_seen++; }

void begin() { s_seen = 0; s_windowAt = 0; s_rate = 0; }

void tick(uint32_t now) {
    if (!s_windowAt) { s_windowAt = now; return; }
    const uint32_t dur = now - s_windowAt;
    if (dur < 5000) return;                       // a five-second window
    s_rate = (uint16_t)((uint64_t)s_seen * 10000ull / dur);
    s_seen = 0;
    s_windowAt = now;
}

bool concluded() { return true; }                 // it did; see above

Stats stats() {
    Stats st{};
    st.offRate = s_rate;                          // the live rate now
    st.onRate  = 0;
    st.deltaPct = 0;
    st.cycles = 0;
    st.advOn  = Mesh::advertising();
    st.advMs  = 1500;
    st.heapFreeKb  = heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024;
    st.heapBlockKb = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024;
    return st;
}

} // namespace MeshProbe
#endif

#if SQUACH_MESH
#include "meshmsg.h"
#include "meshtalk.h"

// SquachMesh's radio half: our advert, our scan response and our own
// address. Everything that does not touch NimBLE -- who is visiting, the
// decoder, where a message frame goes -- is src/mesh.cpp, so that the
// emulator compiles the same code; sim/meshsim.cpp is its radio half.
namespace Mesh {

static bool            s_advOn      = false;
static uint32_t        s_advAt      = 0;
// Which outgoing message the scan response carries right now; 0 is none.
static uint32_t        s_srGen      = 0;
static bool            s_macSet     = false;
static const uint16_t  ADV_MS       = 1500;

bool advertising() { return s_advOn; }

// The payload we last handed the stack, so an unchanged one is never handed
// over twice.
static uint8_t s_lastAdv[SquachMesh::LEN_MAX];
static size_t  s_lastAdvLen = 0;

static void setAdvertising(bool on, uint32_t now) {
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (!adv) return;
    if (!on) { if (s_advOn) adv->stop(); s_advOn = false; return; }

    uint8_t buf[SquachMesh::LEN_MAX];
    const size_t n = buildSelf(buf);

    // Only touch the stack when the advert actually differs.
    //
    // This was every ten seconds unconditionally, and each pass built a
    // std::string and a NimBLEAdvertisementData and pushed them into NimBLE
    // -- roughly 360 allocation cycles an hour, into a heap that a one-hour
    // soak showed dropping from a 17 KB largest free block to 1.4 KB. That
    // is the fragmentation the broadcaster role was disabled to avoid in the
    // first place, arriving by a route this file created.
    //
    // The point of re-advertising was that a changed outfit or name should
    // propagate without a reboot. Comparing gets that for one memcmp and
    // no allocation at all on the pass where nothing changed, which is
    // every pass but the rare one.
    // The scan response counts as part of "the advert" here: a message
    // starting or expiring is a change, and nothing else is.
    size_t   outLen = 0;
    uint32_t outGen = 0;
    const uint8_t* out = MeshTalk::outgoing(now, outLen, outGen);
    const bool same = (n == s_lastAdvLen) && (memcmp(buf, s_lastAdv, n) == 0) &&
                      outGen == s_srGen;
    if (same && s_advOn) return;

    std::string md;
    md.reserve(n + 2);
    md.push_back((char)(SquachMesh::COMPANY_ID & 0xFF));
    md.push_back((char)(SquachMesh::COMPANY_ID >> 8));
    md.append((const char*)buf, n);

    NimBLEAdvertisementData d;
    d.setManufacturerData(md);
    if (s_advOn) adv->stop();
    adv->setAdvertisementData(d);
    // A message goes in the scan response, and turning the scan response ON
    // is also what makes the advert scannable at all: setScanResponseData()
    // alone stores the bytes and leaves the advert non-scannable, so nobody
    // would ever ask for them. Off again the moment the message expires.
    if (out) {
        std::string sm;
        sm.reserve(outLen + 2);
        sm.push_back((char)(SquachMesh::COMPANY_ID & 0xFF));
        sm.push_back((char)(SquachMesh::COMPANY_ID >> 8));
        sm.append((const char*)out, outLen);
        NimBLEAdvertisementData r;
        r.setManufacturerData(sm);
        adv->setScanResponseData(r);
        adv->enableScanResponse(true);
        // Scannable: a peer's scan request is how the message travels.
        adv->setDiscoverableMode(BLE_GAP_DISC_MODE_GEN);
    } else {
        // 2.x pushes scan-response data to the controller the moment it is
        // set and enableScanResponse(false) only clears a flag, so the last
        // message would go on being served to anyone who asked. So the
        // advert goes non-scannable (non-connectable and non-discoverable
        // is ADV_NONCONN_IND): the controller answers no scan request in
        // that mode, so the stale reply is never sent, and no scanner pays
        // for a request that has nothing behind it. The next message sets
        // fresh reply data before the advert turns scannable again. (Not
        // cleared with empty data: the library takes &payload[0] of it.)
        adv->enableScanResponse(false);
        adv->setDiscoverableMode(BLE_GAP_DISC_MODE_NON);
    }
    // Never connectable. Update mode's server shares this advertiser, and a
    // stack with the peripheral role compiled in defaults to connectable.
    adv->setConnectableMode(BLE_GAP_CONN_MODE_NON);
    adv->setMinInterval((uint16_t)(ADV_MS * 8 / 5));
    adv->setMaxInterval((uint16_t)(ADV_MS * 8 / 5 + 16));
    adv->start();

    memcpy(s_lastAdv, buf, n);
    s_lastAdvLen = n;
    s_advOn = true;
    s_srGen = outGen;
}

void radioTick(uint32_t now) {
    // Update mode owns the advertiser; see DetectionEngine::startUpdateRadio().
    // Resting radios have nothing to advertise with unless BLE stayed up.
    if (g_rawMode == RawScanMode::UPDATE) return;
    if (g_rawMode == RawScanMode::REST && g_restBle) return;
    // Our own address goes into the nonce of every message we send, so the
    // runtime needs it -- read once, after the stack is up, and copied out of
    // a named NimBLEAddress rather than through a pointer into a temporary.
    if (!s_macSet) {
        const NimBLEAddress a = NimBLEDevice::getAddress();
        MeshTalk::setOwnMac(a.getBase()->val);
        s_macSet = true;
    }

    const bool want = Settings::meshTransmit() && !Research::active();
    // Polled rather than event-driven, but setAdvertising() now returns on a
    // memcmp when nothing changed, so this costs one comparison every ten
    // seconds instead of rebuilding the advert 360 times an hour. A message
    // starting or expiring is checked every tick, though: nobody should wait
    // ten seconds for "On my way." to go out.
    size_t   ol = 0;
    uint32_t og = 0;
    MeshTalk::outgoing(now, ol, og);
    if (want && (!s_advOn || (now - s_advAt) > 10000 || og != s_srGen)) {
        setAdvertising(true, now);
        s_advAt = now;
    }
    if (!want && s_advOn) setAdvertising(false, now);
}

void stopAdvertisingForUpdate() { setAdvertising(false, 0); }

} // namespace Mesh
#endif

// ---- the scan-result flush --------------------------------------------------
// The scan is restarted once a minute. Under the old library this was the
// only thing that ever freed a record for a device that never answered a
// scan request; the 200 ms reply timeout set in init() does that now, and
// detection no longer waits on the reply (see onDiscovered). What the
// restart still does: it clears the duplicate cache and any record the
// timeout has not reached, and it is the moment a mode change or a WINDOW
// command takes effect. It costs the adverts of the stop/start gap, and any
// device still inside its 200 ms wait at that moment is dropped from the
// library's list unreported -- which is why first-sight detection matters.
static const uint32_t SCAN_FLUSH_MS = 60000;
// The minute was sized at a workplace where the list grew 480 bytes a
// minute. A user's board in a denser place -- 129 adverts a second, and a
// screen photo of DIAGNOSTICS to prove it -- ate the heap in 28 seconds and
// aborted in the host task before the first flush ever came round. So the
// heap is watched too: a flush also goes out the moment the largest free
// block falls under this, no more than once every few seconds.
static const uint32_t SCAN_FLUSH_MIN_MS   = 4000;
// Measured on the bench: a healthy 2.8" board has 12 KB in a piece with
// Bluetooth up, and a board under real pressure sat at 5 KB and below, so
// the bar goes between them. At 20 KB it fired every four seconds at rest.
static const uint32_t SCAN_FLUSH_BLOCK_B  = 8192;
// A breath after the radios come up, no more: the same user's board, with
// fifteen seconds here, was dead at two seconds up.
static const uint32_t SCAN_FLUSH_SETTLE_MS = 1000;
// And when flushing is not enough -- the same user's board, on the first
// fix, flushed six times in 28 seconds, 9 KB each, and sat at 14 KB free
// with a 5 KB largest block between them -- the scan goes PASSIVE for a
// while. A passive scan sends no scan requests, so nothing waits on a
// scan response and the list cannot grow at all. What it costs is the
// scan responses themselves: the names some devices only give when asked.
// Two minutes of that, then active again, and again if it comes to it.
static const uint32_t SCAN_PASSIVE_MS     = 120000;
static const uint8_t  SCAN_PRESSED_LIMIT  = 3;       // pressed flushes inside a minute
// And the rate itself decides, before the heap ever has to: an active scan
// keeps a record for every device it has asked and not yet heard back from,
// and at 184 adverts a second (a college IT floor, measured on a user's
// board) those records ate a 14 KB block in under a second. So the scan is
// passive until the room has been quiet for a while, active while it stays
// quiet, and passive again the moment it gets loud. A passive scan misses
// only what a device says when asked: some names, and a squad message.
static const uint32_t SCAN_ACTIVE_BELOW   = 50;      // adverts/s: quiet enough to ask
// Raised from 90 to 300 on 2.x: with the 200 ms reply timeout an active scan
// costs about 3 KB under a 200-a-second flood (measured, the fake flood plus
// a Flipper), so a busy room keeps its names and squad messages. Passive is
// still the net above this, and heap pressure still forces it regardless.
static const uint32_t SCAN_PASSIVE_ABOVE  = 300;     // adverts/s: too loud to keep asking
// The block an active scan needs to spare before it starts. Measured on the
// bench: a 2.8" board has 12 KB in a piece once Bluetooth is up, and ran
// active scans on that for a year, so the bar sits under it.
static const uint32_t SCAN_ACTIVE_BLOCK_B = 8192;
static const uint32_t SCAN_MODE_SETTLE_MS = 5000;    // listen this long before the first change
static const uint32_t SCAN_MODE_DWELL_MS  = 30000;   // and this long between changes
static volatile bool  s_wantPassive = true;          // the loop task's decision, read on the host task
static uint32_t       s_advRate     = 0;             // adverts/s over the last second
static uint32_t       s_lastFlush   = 0;
static uint32_t       s_pressedAt[SCAN_PRESSED_LIMIT] = { 0, 0, 0 };
static uint8_t        s_pressedIx   = 0;
static volatile uint32_t s_passiveUntil = 0;        // read on the host task
static bool           s_passiveNow  = true;          // what the host task last set: passive from init()
bool scanPassiveNow() { return s_passiveNow; }
static ScanFlushStats s_flush       = { 0, 0, 0 };   // written on the host task
static uint32_t       s_flushLogged = 0;
static struct ble_npl_event s_flushEv;
static bool           s_flushEvReady = false;

ScanFlushStats scanFlushStats() { return s_flush; }

// Runs on the NimBLE host task.
static void scanFlushOnHost(struct ble_npl_event*) {
    NimBLEScan* scan = NimBLEDevice::getScan();
    // Checked again here: a raw scan may have started, or scanning been
    // switched off, between the post and now.
    if ((g_rawMode != RawScanMode::NONE && !(g_rawMode == RawScanMode::REST && !g_restBle)) ||
        !scan || !scan->isScanning()) return;
    // Measured across the stop alone -- start() may allocate, and that is not
    // what this number is for.
    const uint32_t before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    scan->stop();
    const uint32_t after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    // Active or passive, as the loop task decided; the mode only takes at
    // a start, which is why the decision is carried in here.
    const uint32_t nowMs = millis();
    (void)nowMs;
    if (s_intervalReq) { scan->setInterval(s_intervalReq); s_intervalReq = 0; }
    if (s_windowReq) { scan->setWindow(s_windowReq); s_windowReq = 0; }
    const bool passive = s_wantPassive;
    if (passive != s_passiveNow) { scan->setActiveScan(!passive); s_passiveNow = passive; }
    scan->start(0, false, true);
    const uint32_t freed = (after > before) ? after - before : 0;
    s_flush.lastFreed   = freed;
    s_flush.totalFreed += freed;
    s_flush.count++;
}

// Once a second: the advert rate, and whether the scan should be asking.
// Returns true when the mode changed and the scan wants a restart.
static bool scanModeTick(uint32_t now, uint32_t largest) {
    static uint32_t lastAt = 0, lastRaw = 0, modeSince = 0;
    if (now - lastAt < 1000) return false;
    const uint32_t raw = s_advRaw;
    s_advRate = (raw - lastRaw) * 1000UL / (now - lastAt);
    lastRaw = raw;
    lastAt  = now;
    const bool pressedWindow = s_passiveUntil && (int32_t)(s_passiveUntil - now) > 0;
    bool want = s_wantPassive;
    // Heap pressure first: the bench may pin the scan active to watch it
    // suffer, but a board that is out of room goes passive whatever the pin
    // says, since the pin ships in every build and the abort is real. That
    // is the pressed window (three early flushes in a minute) and the same
    // block bar that gates going active in AUTO.
    if (Research::active() || largest < SCAN_ACTIVE_BLOCK_B || heap_caps_get_free_size(MALLOC_CAP_8BIT)<12288) want = true;
    else if (pressedWindow)       want = true;
    else if (s_scanPin == 1) {
        // Pinned active, while there is room: under the bar it goes passive
        // at once, and comes back only after the same dwell AUTO keeps, or
        // a flood that hovers at the bar would restart the scan every second.
        if (largest < SCAN_ACTIVE_BLOCK_B) want = true;
        else if (!s_wantPassive || now - modeSince >= (modeSince ? SCAN_MODE_DWELL_MS : SCAN_MODE_SETTLE_MS)) want = false;
    }
    else if (s_scanPin == 2) want = true;
    else if (!s_wantPassive && s_advRate > SCAN_PASSIVE_ABOVE) want = true;
    else if (s_wantPassive && now - modeSince >= (modeSince ? SCAN_MODE_DWELL_MS : SCAN_MODE_SETTLE_MS) &&
             s_advRate < SCAN_ACTIVE_BELOW && largest >= SCAN_ACTIVE_BLOCK_B) want = false;
    if (want == s_wantPassive) return false;
    s_wantPassive = want;
    modeSince = now;
    Serial.printf("[scan] %s: %lu adverts/s, largest block %lu\n",
                  want ? "passive, the room is loud" : "active, the room is quiet",
                  (unsigned long)s_advRate, (unsigned long)largest);
    return true;
}

uint32_t advertRate() { return s_advRate; }

static void scanFlushTick() {
    // Printed from here rather than the host task, which should never be kept
    // waiting on the UART.
    if (s_flush.count != s_flushLogged) {
        s_flushLogged = s_flush.count;
        Serial.printf("[scan] restart %lu freed %lu B (%lu total), heap %lu, largest %lu, dropped %lu\n",
                      (unsigned long)s_flush.count, (unsigned long)s_flush.lastFreed,
                      (unsigned long)s_flush.totalFreed, (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                      (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                      (unsigned long)s_advertsDropped);
    }
    const uint32_t now = millis();
    const uint32_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    s_heapLow = largest < 1536;
    // Startup telemetry is deliberately allocation-free and contains no device
    // identifiers. It lets a serial capture distinguish a steady low baseline
    // from progressive fragmentation on the actual CYD with its SD inserted.
    static uint32_t heapLoggedAt = 0;
    const uint32_t heapInterval = now < 90000 ? 5000 : 60000;
    if (now-heapLoggedAt >= heapInterval) {
        heapLoggedAt = now;
        Serial.printf("[heap] up %lu free %lu largest %lu adverts %lu dropped %lu\n",
                      (unsigned long)now, (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                      (unsigned long)largest, (unsigned long)advertsSeen(),
                      (unsigned long)s_advertsDropped);
    }
    bool modeChanged = scanModeTick(now, largest);
    if (s_windowPending) { s_windowPending = false; modeChanged = true; Serial.printf("[scan] window %u from the next restart\n", (unsigned)s_windowReq); }
    // A passive scan holds nothing between flushes, so heap pressure there
    // is somebody else's and a restart would not help.
    const bool pressed = !s_passiveNow && largest < SCAN_FLUSH_BLOCK_B && now > SCAN_FLUSH_SETTLE_MS &&
                         now - s_lastFlush >= SCAN_FLUSH_MIN_MS;
    if (!pressed && !modeChanged && now - s_lastFlush < SCAN_FLUSH_MS) return;
    if (pressed) {
        Serial.printf("[scan] heap pressed: largest block %lu, flushing early\n", (unsigned long)largest);
        // Three pressed flushes inside a minute: flushing is losing, go passive.
        s_pressedAt[s_pressedIx] = now;
        s_pressedIx = (uint8_t)((s_pressedIx + 1) % SCAN_PRESSED_LIMIT);
        bool allRecent = true;
        for (uint8_t i = 0; i < SCAN_PRESSED_LIMIT; i++)
            if (!s_pressedAt[i] || now - s_pressedAt[i] > 60000) allRecent = false;
        if (allRecent && !(s_passiveUntil && (int32_t)(s_passiveUntil - now) > 0)) {
            s_passiveUntil = now + SCAN_PASSIVE_MS;
            for (uint8_t i = 0; i < SCAN_PRESSED_LIMIT; i++) s_pressedAt[i] = 0;
            Serial.printf("[scan] passive for %lu s: too many devices for the heap here\n",
                          (unsigned long)(SCAN_PASSIVE_MS / 1000));
        }
    }
    s_lastFlush = now;
    NimBLEScan* scan = NimBLEDevice::getScan();
    // Never restart a scan that is not running: that would be switching
    // Bluetooth scanning back on behind whoever turned it off.
    if (!scan || !scan->isScanning()) return;
    if (!s_flushEvReady) {
        ble_npl_event_init(&s_flushEv, scanFlushOnHost, nullptr);
        s_flushEvReady = true;
    }
    // A post while the last one is still queued is ignored by NimBLE's port.
    ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &s_flushEv);
}

void DetectionEngine::loop() {
    if (_stopping.load() || g_rawMode == RawScanMode::UPDATE) return;
    static int researchMode = -1;
    static uint32_t coverageAt = 0;
    const uint32_t coverageNow = millis();
    int wanted = Research::active() ? (int)Research::profile() : -1;
    if (wanted < 0 && g_rawMode == RawScanMode::NONE && DroneWatch::focused())
        wanted = DroneWatch::wifiPhase(coverageNow) ? (int)Research::Profile::WIFI : (int)Research::Profile::BLUETOOTH;
    if (wanted != researchMode) {
        NimBLEScan* scan = NimBLEDevice::getScan();
        if (scan) {
            scan->stop();
            if (wanted >= 0) { s_wantPassive = s_passiveNow = true; scan->setActiveScan(false); }
            if (wanted != (int)Research::Profile::WIFI) scan->start(0, false, true);
        }
        if (wanted >= 0) { esp_wifi_disconnect(); esp_wifi_set_mode(WIFI_MODE_NULL); }
        else esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_promiscuous(wanted != (int)Research::Profile::BLUETOOTH);
        researchMode = wanted;
        coverageAt = coverageNow;
    }
    if (Research::active()) {
        auto* scan = NimBLEDevice::getScan();
        Research::coverage(coverageNow - coverageAt, scan && scan->isScanning(),
                           wanted != (int)Research::Profile::BLUETOOTH, _wifiChannel);
    }
    coverageAt = coverageNow;
    if (g_rawMode != RawScanMode::NONE && g_rawMode != RawScanMode::REST) {
        // A raw scan owns the radio right now -- channel hopping here
        // would fight WiFi.scanNetworks()'s own hopping during a WIFI
        // sweep, and the WiFi promiscuous queue is empty anyway (it's
        // disabled for the duration of either raw mode, see
        // startRawBleScan/startRawWifiScan).
        _sd.tick();
        return;
    }
    // After the raw-scan return above, so a raw scan that owns the radio is
    // never restarted out from under it.
    scanFlushTick();
    Detection incoming;
    for (uint8_t i = 0; i < 16 && _blePending.pop(incoming, millis()); ++i)
        recordObservation(incoming);
    if (g_rawMode != RawScanMode::REST) {
        if (wanted != (int)Research::Profile::BLUETOOTH) hopChannel();
        processWiFiQ();
        processDeauthQ();
    }
    expireStale();
    // Detect rapid address churn before those short-lived rows have time to
    // expire. This is the upstream v1.22 tag-spam mitigation: it suppresses
    // repeated popups, while detections continue to be logged.
    {
        static uint32_t at=0;
        static uint16_t seen[SpamWatch::TYPES]{};
        const uint32_t now=millis();
        if(now-at>=60000){
            at=now;
            for(uint8_t k=0;k<SpamWatch::TYPES;k++){
                const uint16_t n=_newBle[k],fresh=(uint16_t)(n-seen[k]);seen[k]=n;
                if(_spam.noteBurst(k,fresh,now))
                    Serial.printf("[spam] %s flood: %u new addresses in a minute\n",detectionTypeName((DetectionType)k),(unsigned)fresh);
            }
        }
    }
    decayChannelActivity();
    saveLifetime(millis());
    drainBlackBox(millis());
    drainSd(millis());
    _sd.logPressure(alerts.dropped(), _blePending.dropped());
    _sd.tick();
    ScanProfile::tick(millis(),wifiFramesSeen(),advertsSeen(),_channelSweeps,_storedEvents,
                      _blePending.dropped()+alerts.dropped(),heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void DetectionEngine::decayChannelActivity() {
    uint32_t now = millis();
    for (uint8_t ch = 1; ch <= 13; ch++) {
        if (_channelActivity[ch] > 0 && now - _channelLastMs[ch] > 200) {
            _channelActivity[ch] = (_channelActivity[ch] > 4) ? _channelActivity[ch] - 4 : 0;
            _channelLastMs[ch] = now;
        }
    }
}

void DetectionEngine::hopChannel() {
    // Dwell ~300ms per channel, cycling 1-13 — without this the
    // promiscuous sniffer stays parked on whatever channel the radio
    // defaulted to and only ever sees traffic on that one channel,
    // missing anything (real hardware included, not just test rigs)
    // transmitting elsewhere in the band.
    uint32_t now = millis();
    const bool focus = DroneWatch::focused() && !Research::active();
    const uint8_t preferred = focus ? DroneWatch::preferredChannel(now) : 0;
    if (preferred && _wifiChannel == preferred) return;
    if (now - _lastHopMs < (focus ? 900u : _dwellMs)) return;
    if (!focus) {
        const uint32_t spent = now - _lastHopMs;
        const uint32_t frames = s_chanFrames[_wifiChannel];
        s_chanFrames[_wifiChannel] = 0;
        if (spent <= 2u * _dwellMs + 200u) {
            uint32_t rate = frames * 16000u / (spent ? spent : 1u);
            if (rate > 0xFFFFu) rate = 0xFFFFu;
            s_chanRate[_wifiChannel] = (uint16_t)(((uint32_t)s_chanRate[_wifiChannel] * 3u + rate) / 4u);
        }
    }
    _lastHopMs = now;
    wifi_country_t country{};
    uint8_t first = 1, last = 11;
    if (esp_wifi_get_country(&country) == ESP_OK && country.schan >= 1 && country.nchan &&
        country.schan + country.nchan - 1 <= 13) {
        first = country.schan; last = country.schan + country.nchan - 1;
    }
    uint8_t next = preferred ? preferred : (_wifiChannel < first || _wifiChannel >= last ? first : _wifiChannel + 1);
    if (next < first || next > last) next = first;
    if(!preferred&&next==first&&_wifiChannel>=first)++_channelSweeps;
    if (!focus) {
        uint16_t top = 0;
        for (uint8_t ch = first; ch <= last; ++ch) if (s_chanRate[ch] > top) top = s_chanRate[ch];
        uint16_t totalShares = 0, nextShares = 2;
        for (uint8_t ch = first; ch <= last; ++ch) {
            const uint16_t rate = s_chanRate[ch];
            const uint16_t shares = !top ? 2 : (rate * 4u >= top ? 4 : (rate ? 2 : 1));
            totalShares += shares;
            if (ch == next) nextShares = shares;
        }
        const uint32_t budget=ScanProfile::wifiCycleMs();
        _dwellMs = totalShares ? (uint16_t)(budget * nextShares / totalShares) : (uint16_t)(budget/13u);
        const uint16_t floor=(budget<=2200u)?80u:120u;
        if (_dwellMs < floor) _dwellMs = floor;
        s_chanFrames[next] = 0;
    }
    const bool ok = esp_wifi_set_channel(next, WIFI_SECOND_CHAN_NONE) == ESP_OK;
    uint8_t actual = 0; wifi_second_chan_t secondary;
    if (esp_wifi_get_channel(&actual, &secondary) == ESP_OK) _wifiChannel = actual;
    DroneWatch::channelResult(_wifiChannel, ok);
}

void DetectionEngine::clearLog() {
    alerts.clear();
    _blePending.clear();
    _logCount = 0;
    _logHead  = 0;
    _latest   = nullptr;
    for (uint8_t i = 0; i < (uint8_t)DetectionType::COUNT; i++) {
        _typeCounts[i] = 0;
    }
}

void IRAM_ATTR DetectionEngine::postWiFi(const uint8_t* mac, int8_t rssi, uint8_t channel,
                                         const char* ssid, bool encrypted,
                                         bool pwnagotchi, bool drone, AddressRole role) {
    if (_stopping.load()) return;
    if (!mac) return;
    // Group-addressed (broadcast/multicast) destinations can never be a
    // real device: bit 0 of byte 0 is the I/G bit, and every OUI in
    // kOuiTable is a globally-administered unicast prefix with it
    // clear, so these could only ever fall through lookupOui() as
    // UNKNOWN. Rejected here rather than in processWiFiQ() because the
    // queue only holds 7 entries and is drained once per rendered
    // frame -- a slot spent on ff:ff:ff:ff:ff:ff or 01:00:5e:... is a
    // slot a real beacon can't have.
    //
    // This costs the RF-spectrum background's channel-activity level
    // nothing: the data-frame path that produces these is the same one
    // that also posts addr2 (the transmitter, always unicast) with an
    // identical rssi/channel, so every frame still lands in that
    // running maximum exactly once.
    if (mac[0] & 0x01) return;
    uint8_t next = (_wifiQHead + 1) % WIFI_Q_CAP;
    if (next == _wifiQTail) return;            // queue full, drop
    WiFiQEntry& e = (WiFiQEntry&)_wifiQ[_wifiQHead];
    memcpy((void*)e.mac, mac, 6);
    e.rssi    = rssi;
    e.channel = channel;
    if (ssid && ssid[0]) {
        strncpy((char*)e.ssid, ssid, sizeof(e.ssid) - 1);
        e.ssid[sizeof(e.ssid) - 1] = 0;
    } else {
        e.ssid[0] = 0;
    }
    e.encrypted = encrypted;
    e.pwnagotchi = pwnagotchi;
    e.drone = drone;
    e.role = role;
    _wifiQHead = next;
}

void IRAM_ATTR DetectionEngine::postDeauth(const DeauthFrameEvidence& frame) {
    if (_stopping.load()) return;
    uint8_t next = (_deauthQHead + 1) % DEAUTH_Q_CAP;
    if (next == _deauthQTail) return;           // queue full, drop
    DeauthQEntry& e = (DeauthQEntry&)_deauthQ[_deauthQHead];
    memcpy((void*)&e.frame, &frame, sizeof frame);
    _deauthQHead = next;
}

void DetectionEngine::processDeauthQ() {
    while (_deauthQTail != _deauthQHead) {
        DeauthQEntry e;
        {
            noInterrupts();
            e = (const DeauthQEntry&)_deauthQ[_deauthQTail];
            _deauthQTail = (_deauthQTail + 1) % DEAUTH_Q_CAP;
            interrupts();
        }

        const uint32_t now = millis();
        const DeauthBurstResult burst = _deauthTracker.note(e.frame, now);
        if (burst.alert) Research::noteDeauthBurst(burst.distinctTargets);

        // Tracker bookkeeping runs while the type is disabled too. This
        // consumes any threshold crossing and its per-source cooldown, so
        // re-enabling cannot emit a stale burst immediately.
        if (burst.alert && Settings::typeEnabled(DetectionType::DEAUTH)) {
            Detection d;
            memset(&d, 0, sizeof(d));
            memcpy(d.mac, burst.source, 6);
            d.rssi    = burst.rssi;
            d.channel = burst.channel;
            d.type    = DetectionType::DEAUTH;
            d.evidence = MatchEvidence::DEAUTH_BURST;
            d.addressRole = AddressRole::TRANSMITTER;
            // Multiple receivers make a flood interpretation stronger, but
            // even HIGH here describes the observed pattern, not an
            // authenticated attacker identity.
            d.conf    = burst.distinctTargets > 1 ? Confidence::HIGH_CONF
                                                   : confidenceFor(DetectionType::DEAUTH);
            d.vendor = "WiFi frames";
            d.firstSeen = burst.firstMs;
            d.lastSeen  = burst.lastMs;
            if (burst.sameBssid)
                snprintf(d.name, sizeof d.name, "%02X:%02X:%02X:%02X:%02X:%02X",
                         burst.bssid[0], burst.bssid[1], burst.bssid[2],
                         burst.bssid[3], burst.bssid[4], burst.bssid[5]);
            d.signature = burst.reason;
            d.evidenceBits = (uint16_t)(burst.distinctTargets & DEAUTH_META_TARGET_MASK);
            if (burst.reasonValid)    d.evidenceBits |= DEAUTH_META_REASON_VALID;
            if (burst.protectedSeen)  d.evidenceBits |= DEAUTH_META_PROTECTED_SEEN;
            if (burst.unprotectedSeen)d.evidenceBits |= DEAUTH_META_UNPROTECTED_SEEN;
            if (burst.sameBssid)      d.evidenceBits |= DEAUTH_META_SAME_BSSID | DEAUTH_META_BSSID_VALID;
            // hits doubles as "how many frames triggered this" here,
            // rather than a repeat-sighting count like every other
            // type uses it for -- there's no single persistent device
            // identity behind a flood the way there is for a tracker
            // or camera.
            d.hits   = burst.count;
            d.active = true;
            // A later burst with the same claimed source updates that row.
            // This is log coalescing, not a claim that both bursts came from
            // the same physical hardware; the address may be spoofed.
            for (uint8_t i = 0; i < _logCount; i++) {
                const uint8_t slot = (_logHead + LOG_CAP - 1 - i) % LOG_CAP;
                Detection& row = _log[slot];
                if (row.type != DetectionType::DEAUTH || memcmp(row.mac, burst.source, 6) != 0) continue;
                row.locationKey=LocationLabel::currentKey();
                row.prevRssi  = row.rssi;
                row.rssi      = d.rssi;
                row.channel   = d.channel;
                row.hits      = d.hits;
                row.lastSeen  = d.lastSeen;
                row.firstSeen = d.firstSeen;
                row.conf      = d.conf;
                row.evidence  = d.evidence;
                row.addressRole = d.addressRole;
                row.evidenceBits = d.evidenceBits;
                row.signature = d.signature;
                row.vendor    = d.vendor;
                memcpy(row.name, d.name, sizeof row.name);
                row.restored  = 0;
                if (!row.active) {
                    row.active = true;
                    _typeCounts[(uint8_t)DetectionType::DEAUTH]++;
                }
                Bingo::note(DetectionType::DEAUTH);
                Dex::note(DetectionType::DEAUTH, d.rssi);
                Regulars::note(d.mac, DetectionType::DEAUTH);
                _latest = &row;
                _latestChangeMs = now;
                alerts.push(row);
                queueBlackBox(row, true);
                return;
            }
            pushLog(d);
        }
    }
}

void DetectionEngine::mergeRemoteId(const uint8_t* mac, const uint8_t* payload,
                                    uint8_t len) {
    if (!mac || !payload) return;
    // A different aircraft means the accumulated record is no longer about
    // the same object, and half of one drone merged onto half of another
    // would read as a plausible aircraft that does not exist.
    if (memcmp(mac, _ridMac, 6) != 0) {
        RemoteId::reset(_rid);
        memcpy(_ridMac, mac, 6);
    }
    RemoteId::merge(payload, len, _rid, millis());
}

void DetectionEngine::postBle(Detection d) {
    if (_stopping.load()) return;
    // The NimBLE task never mutates the history while the main loop reads it.
    // Copy into a bounded queue; merge, count and persist only on the main task.
    if (d.type != DetectionType::UNKNOWN && (uint8_t)d.type < (uint8_t)DetectionType::COUNT)
        _blePending.push(d);
}

void DetectionEngine::recordObservation(Detection d) {
    d.locationKey=LocationLabel::currentKey();
    // Disabled types (Settings > DETECTION FILTER) are dropped here,
    // before the dedupe/merge below -- that merge branch re-activates
    // and re-alerts on an already-logged device without ever reaching
    // pushLog(), so gating pushLog() alone would miss it. An entry
    // already in the log for a type disabled after the fact isn't
    // touched or removed; it just stops updating and ages out through
    // the normal expireStale() path like any other device that goes
    // out of range.
    if (d.type == DetectionType::UNKNOWN || (uint8_t)d.type >= (uint8_t)DetectionType::COUNT || !Settings::typeEnabled(d.type)) return;
    // Try to dedupe / merge with existing log entry by MAC
    for (uint8_t i = 0; i < _logCount; i++) {
        uint8_t slot = (_logHead + LOG_CAP - 1 - i) % LOG_CAP;
        if (memcmp(_log[slot].mac, d.mac, 6) == 0 &&
            _log[slot].type == d.type) {
            const bool returning = !_log[slot].active;
            const bool notify = mergeObservation(_log[slot], d, millis());
            _log[slot].locationKey=d.locationKey;
            Bingo::note(d.type);
            Dex::note(d.type, d.rssi);
            Regulars::note(d.mac, d.type);
            if (returning) {
                _typeCounts[(uint8_t)d.type]++;
                _latest = &_log[slot];
                _latestChangeMs = millis();
            }
            if (notify) {
                alerts.push(_log[slot]);
                queueBlackBox(_log[slot], returning);
            }
            return;
        }
    }
    pushLog(d);
}

// Nothing calls this today (the Classic inquiry was never switched on), but
// if something does, it gets the same MAC+type merge as BLE rather than a
// new row per sighting.
void DetectionEngine::postBtClassic(Detection d) {
    postBle(d);
}

void DetectionEngine::postRawBle(RawBleResult r) {
    // Looked up by MAC and updated in place -- this is "everything
    // currently visible", not a chronological log, so a device seen
    // again just refreshes its existing row instead of duplicating it.
    for (uint8_t i = 0; i < _rawBleCount; i++) {
        if (memcmp(_rawBle[i].mac, r.mac, 6) == 0) {
            _rawBle[i].prev = _rawBle[i].rssi;   // for the NEARBY list's closer/further arrow
            _rawBle[i].rssi = r.rssi;
            if (r.name[0]) strncpy(_rawBle[i].name, r.name, sizeof(_rawBle[i].name) - 1);
            return;
        }
    }
    if (_rawBleCount < RAW_BLE_CAP) {
        r.prev = r.rssi;
        _rawBle[_rawBleCount++] = r;
    }
    // else: full -- ignore further new devices until the next
    // startRawBleScan() resets the list. RAW_BLE_CAP entries is plenty
    // for a single focused sweep and keeps this bounded regardless of
    // how many devices happen to be nearby.
}

void DetectionEngine::startRawBleScan() {
    if (g_rawMode == RawScanMode::WIFI) WiFi.scanDelete();
    _rawBleCount = 0;
    // The continuous NimBLE scan (started once, forever, in init())
    // keeps running -- onResult() just routes into postRawBle() above
    // instead of the signature matcher while g_rawMode == BLE. WiFi's
    // promiscuous capture is switched off so the radio is focused on
    // BLE for the duration, per the "pause the continuous scan and
    // focus on what we're scanning for" design.
    esp_wifi_set_promiscuous(false);
    g_rawMode = RawScanMode::BLE;
    g_rawBleStartMs = millis();
}

bool DetectionEngine::rawBleScanDone() const {
    return g_rawMode == RawScanMode::BLE && (millis() - g_rawBleStartMs) >= RAW_BLE_SCAN_MS;
}

const RawBleResult* DetectionEngine::rawBleAt(uint8_t idx) const {
    if (idx >= _rawBleCount) return nullptr;
    return &_rawBle[idx];
}

void DetectionEngine::startRawWifiScan() {
    // Gate the BLE callback off first (it'd otherwise still be live
    // during the scan) before touching the radio.
    g_rawMode = RawScanMode::WIFI;
    esp_wifi_set_promiscuous(false);
    WiFi.scanNetworks(true /* async */);
}

bool DetectionEngine::rawWifiScanDone() const {
    return g_rawMode == RawScanMode::WIFI && WiFi.scanComplete() >= 0;
}

uint8_t DetectionEngine::rawWifiCount() const {
    if (!rawWifiScanDone()) return 0;
    int n = WiFi.scanComplete();
    return n > 0 ? (uint8_t)n : 0;
}

const char* DetectionEngine::rawWifiSsid(uint8_t idx) const {
    // WiFi.SSID() returns a temporary String -- copy into a static
    // buffer rather than returning a pointer into it (same pattern as
    // macFmt() below).
    static char buf[33];
    buf[0] = 0;
    if (idx < rawWifiCount()) {
        String s = WiFi.SSID(idx);
        strncpy(buf, s.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = 0;
        if (buf[0] == 0) strncpy(buf, "(hidden)", sizeof(buf) - 1);
    }
    return buf;
}

int8_t DetectionEngine::rawWifiRssi(uint8_t idx) const {
    return idx < rawWifiCount() ? (int8_t)WiFi.RSSI(idx) : 0;
}

uint8_t DetectionEngine::rawWifiChannel(uint8_t idx) const {
    return idx < rawWifiCount() ? (uint8_t)WiFi.channel(idx) : 0;
}

bool DetectionEngine::rawWifiOpen(uint8_t idx) const {
    return idx < rawWifiCount() && WiFi.encryptionType(idx) == WIFI_AUTH_OPEN;
}

const uint8_t* DetectionEngine::rawWifiBssid(uint8_t idx) const {
    if (!rawWifiScanDone() || idx >= rawWifiCount()) return nullptr;
    return WiFi.BSSID(idx);
}

void DetectionEngine::stopRawScan() {
    if (g_rawMode == RawScanMode::WIFI) WiFi.scanDelete();
    g_rawMode = RawScanMode::NONE;
    esp_wifi_set_promiscuous(true);
}

// ---- Bluetooth update mode --------------------------------------------------
// The scan stops and starts ON THE HOST TASK, for the reason spelled out above
// scanFlushOnHost(): the host hands its records to the scan callbacks on the
// other core, and a stop() from the loop task once freed one mid-callback.
static struct ble_npl_event s_updStopEv;
static struct ble_npl_event s_updStartEv;
static bool                 s_updEvReady = false;

static void updScanStopOnHost(struct ble_npl_event*) {
    NimBLEScan* scan = NimBLEDevice::getScan();
    if (scan && scan->isScanning()) scan->stop();
}

static void updScanStartOnHost(struct ble_npl_event*) {
    NimBLEScan* scan = NimBLEDevice::getScan();
    if (g_rawMode == RawScanMode::NONE && scan && !scan->isScanning()) scan->start(0, false, false);
}

void DetectionEngine::startUpdateRadio() {
    if (!s_updEvReady) {
        ble_npl_event_init(&s_updStopEv,  updScanStopOnHost,  nullptr);
        ble_npl_event_init(&s_updStartEv, updScanStartOnHost, nullptr);
        s_updEvReady = true;
    }
    if (g_rawMode == RawScanMode::WIFI) WiFi.scanDelete();
    g_rawMode = RawScanMode::UPDATE;
    esp_wifi_set_promiscuous(false);
#if SQUACH_MESH
    Mesh::stopAdvertisingForUpdate();
#endif
    ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &s_updStopEv);
}

void DetectionEngine::stopUpdateRadio() {
    if (g_rawMode != RawScanMode::UPDATE) return;
    g_rawMode = RawScanMode::NONE;
    esp_wifi_set_promiscuous(true);
    if (s_updEvReady) ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &s_updStartEv);
}

// The watch's radio duty cycle. WiFi is stopped outright -- a started WiFi
// radio draws most of its power just sitting there, and taking it out of
// promiscuous mode alone saves almost nothing. BLE scanning stops too when
// asked, or keeps going in the BLE-always mode, where trackers walking past
// are the catches that cannot wait. Only from NONE: a raw scan or an update
// owns the radio, and the cycle waits its turn.
bool DetectionEngine::restRadios(bool bleToo) {
    if (g_rawMode != RawScanMode::NONE) return false;
    if (!s_updEvReady) {
        ble_npl_event_init(&s_updStopEv,  updScanStopOnHost,  nullptr);
        ble_npl_event_init(&s_updStartEv, updScanStartOnHost, nullptr);
        s_updEvReady = true;
    }
    g_rawMode = RawScanMode::REST;
    g_restBle = bleToo;
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    if (bleToo) {
#if SQUACH_MESH
        Mesh::stopAdvertisingForUpdate();
#endif
        ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &s_updStopEv);
    }
    return true;
}

void DetectionEngine::wakeRadios() {
    if (g_rawMode != RawScanMode::REST) return;
    g_rawMode = RawScanMode::NONE;
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(_wifiChannel, WIFI_SECOND_CHAN_NONE);
    if (g_restBle && s_updEvReady) ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &s_updStartEv);
    g_restBle = false;
}

bool DetectionEngine::radiosResting() { return g_rawMode == RawScanMode::REST; }

void DetectionEngine::watchBle(const uint8_t* mac, const char* name) {
    _watchKind = WatchKind::BLE;
    memcpy(_watchMac, mac, 6);
    strncpy(_watchLabel, (name && name[0]) ? name : "Unnamed device", sizeof(_watchLabel) - 1);
    _watchLabel[sizeof(_watchLabel) - 1] = 0;
    _watchLastHitMs = 0;
    _watchHitFlag   = false;
    _watchRssiHead = _watchRssiCount = 0;
    _watchRssiLastMs = 0;
}

void DetectionEngine::watchWifi(const uint8_t* bssid, const char* ssid) {
    _watchKind = WatchKind::WIFI;
    memcpy(_watchMac, bssid, 6);
    strncpy(_watchLabel, (ssid && ssid[0]) ? ssid : "(hidden)", sizeof(_watchLabel) - 1);
    _watchLabel[sizeof(_watchLabel) - 1] = 0;
    _watchLastHitMs = 0;
    _watchHitFlag   = false;
    _watchRssiHead = _watchRssiCount = 0;
    _watchRssiLastMs = 0;
}

void DetectionEngine::clearWatch() {
    _watchKind    = WatchKind::NONE;
    _watchHitFlag = false;
    _watchRssiHead = _watchRssiCount = 0;
}

bool DetectionEngine::watchHitPending() {
    if (_watchHitFlag) {
        _watchHitFlag = false;
        return true;
    }
    return false;
}

void DetectionEngine::checkWatchBle(const uint8_t* mac, int8_t rssi) {
    if (_watchKind != WatchKind::BLE) return;
    if (memcmp(mac, _watchMac, 6) != 0) return;
    recordWatchRssi(rssi);
    uint32_t now = millis();
    if (now - _watchLastHitMs < WATCH_COOLDOWN_MS) return;
    _watchLastHitMs = now;
    _watchHitFlag   = true;
}

void DetectionEngine::checkWatchWifi(const uint8_t* mac, int8_t rssi) {
    if (_watchKind != WatchKind::WIFI) return;
    if (memcmp(mac, _watchMac, 6) != 0) return;
    recordWatchRssi(rssi);
    uint32_t now = millis();
    if (now - _watchLastHitMs < WATCH_COOLDOWN_MS) return;
    _watchLastHitMs = now;
    _watchHitFlag   = true;
}

// Throttled independently of WATCH_COOLDOWN_MS above -- that gate is
// about not re-popping the full-screen alert every advertisement,
// this is about building up a dense-enough trend to actually plot.
void DetectionEngine::recordWatchRssi(int8_t rssi) {
    uint32_t now = millis();
    if (now - _watchRssiLastMs < WATCH_RSSI_SAMPLE_MS && _watchRssiCount > 0) return;
    _watchRssiLastMs = now;
    _watchRssiHist[_watchRssiHead] = rssi;
    _watchRssiHead = (_watchRssiHead + 1) % WATCH_RSSI_CAP;
    if (_watchRssiCount < WATCH_RSSI_CAP) _watchRssiCount++;
}

int8_t DetectionEngine::watchRssiAt(uint8_t idx) const {
    if (idx >= _watchRssiCount) return 0;
    // Oldest-first: when the buffer hasn't wrapped yet, oldest is slot
    // 0; once it has, oldest is whatever _watchRssiHead is about to
    // overwrite next.
    uint8_t start = (_watchRssiCount < WATCH_RSSI_CAP) ? 0 : _watchRssiHead;
    uint8_t slot = (start + idx) % WATCH_RSSI_CAP;
    return _watchRssiHist[slot];
}

void DetectionEngine::huntBle(const uint8_t* mac, const char* name) {
    _huntKind = WatchKind::BLE;
    memcpy(_huntMac, mac, 6);
    strncpy(_huntLabel, (name && name[0]) ? name : "Unnamed device", sizeof(_huntLabel) - 1);
    _huntLabel[sizeof(_huntLabel) - 1] = 0;
    _huntRssiHead = _huntRssiCount = 0;
    _huntRssiLastMs = 0;
}

void DetectionEngine::huntWifi(const uint8_t* bssid, const char* ssid) {
    _huntKind = WatchKind::WIFI;
    memcpy(_huntMac, bssid, 6);
    strncpy(_huntLabel, (ssid && ssid[0]) ? ssid : "(hidden)", sizeof(_huntLabel) - 1);
    _huntLabel[sizeof(_huntLabel) - 1] = 0;
    _huntRssiHead = _huntRssiCount = 0;
    _huntRssiLastMs = 0;
}

void DetectionEngine::clearHunt() {
    _huntKind = WatchKind::NONE;
    _huntRssiHead = _huntRssiCount = 0;
}

void DetectionEngine::checkHuntBle(const uint8_t* mac, int8_t rssi) {
    if (_huntKind != WatchKind::BLE) return;
    if (memcmp(mac, _huntMac, 6) != 0) return;
    recordHuntRssi(rssi);
}

void DetectionEngine::checkHuntWifi(const uint8_t* mac, int8_t rssi) {
    if (_huntKind != WatchKind::WIFI) return;
    if (memcmp(mac, _huntMac, 6) != 0) return;
    recordHuntRssi(rssi);
}

void DetectionEngine::recordHuntRssi(int8_t rssi) {
    uint32_t now = millis();
    if (now - _huntRssiLastMs < WATCH_RSSI_SAMPLE_MS && _huntRssiCount > 0) return;
    _huntRssiLastMs = now;
    _huntRssiHist[_huntRssiHead] = rssi;
    _huntRssiHead = (_huntRssiHead + 1) % WATCH_RSSI_CAP;
    if (_huntRssiCount < WATCH_RSSI_CAP) _huntRssiCount++;
}

int8_t DetectionEngine::huntRssiAt(uint8_t idx) const {
    if (idx >= _huntRssiCount) return 0;
    uint8_t start = (_huntRssiCount < WATCH_RSSI_CAP) ? 0 : _huntRssiHead;
    uint8_t slot = (start + idx) % WATCH_RSSI_CAP;
    return _huntRssiHist[slot];
}

// Same vendor, ignoring the locally-administered bit. Multi-SSID and
// mesh APs routinely derive per-radio BSSIDs by setting that bit on
// their base MAC (34:12:98 becomes 36:12:98) -- byte 0 differs by 2 and
// a plain memcmp calls that a different manufacturer, which is how the
// first version of this managed to flag an entire mesh network.
static inline bool sameVendor(const uint8_t* a, const uint8_t* b) {
    return ((a[0] & ~0x02) == (b[0] & ~0x02)) && a[1] == b[1] && a[2] == b[2];
}

// See the AP table's comment in detection.h for why the encryption
// mismatch is the actual test and what it trades away.
//
// One inherent limitation worth naming: whichever BSSID is seen first
// becomes the baseline. If a rogue is already up when you arrive, it
// gets recorded as legitimate and the real AP is what trips the alert.
// The pair is still surfaced either way -- the device is telling you
// two boxes claim one name and disagree about security, which is the
// finding; it can't tell you which of them is lying.
bool DetectionEngine::noteApBeacon(const uint8_t* bssid, const char* ssid, bool encrypted) {
    for (uint8_t i = 0; i < _apCount; i++) {
        if (strncmp(_aps[i].ssid, ssid, sizeof(_aps[i].ssid) - 1) != 0) continue;
        // Same SSID, same BSSID -- just this AP beaconing again. Refresh
        // the posture so a legitimate security change re-baselines
        // rather than alerting forever after.
        if (memcmp(_aps[i].bssid, bssid, 6) == 0) {
            _aps[i].encrypted = encrypted;
            return false;
        }
        // Same hardware vendor -- mesh node, or the other band of the
        // same box. Never flagged, whatever else it says.
        if (sameVendor(_aps[i].bssid, bssid)) return false;
        // Different vendor, but both agree on security. Can't tell a
        // rogue from a mixed-vendor network here, so stay quiet.
        if (_aps[i].encrypted == encrypted) return false;
        // Different vendor AND disagreeing about encryption: one of
        // these two is not what it claims to be.
        return true;
    }
    // First sighting of this SSID: record it as the baseline. Round-
    // robin eviction once full, so a busy area can't grow this without
    // bound.
    ApEntry& slot = (_apCount < AP_CAP) ? _aps[_apCount++] : _aps[_apNext];
    if (_apCount >= AP_CAP) _apNext = (uint8_t)((_apNext + 1) % AP_CAP);
    strncpy(slot.ssid, ssid, sizeof(slot.ssid) - 1);
    slot.ssid[sizeof(slot.ssid) - 1] = 0;
    memcpy(slot.bssid, bssid, 6);
    slot.encrypted = encrypted;
    return false;
}

void DetectionEngine::processWiFiQ() {
    while (_wifiQTail != _wifiQHead) {
        WiFiQEntry e;
        {
            // copy out under volatile guard
            noInterrupts();
            e = (const WiFiQEntry&)_wifiQ[_wifiQTail];
            _wifiQTail = (_wifiQTail + 1) % WIFI_Q_CAP;
            interrupts();
        }
        // Checked for every dequeued frame, regardless of what (if
        // anything) it ends up matching below -- a watched AP's own
        // MAC shows up here as addr2 (probe/data) or addr3/BSSID
        // (beacon), same offsets postWiFi() was already called with.
        checkWatchWifi(e.mac, e.rssi);
        checkHuntWifi(e.mac, e.rssi);
        // Every captured frame feeds the spectrum-waterfall's channel
        // activity level, whether or not it ends up matching anything
        // below — this is meant to reflect real ambient RF traffic,
        // not just known-vendor hits.
        if (e.channel >= 1 && e.channel <= 13) {
            int level = ((int)e.rssi + 90) * 100 / 60;
            if (level < 0) level = 0;
            if (level > 100) level = 100;
            if ((uint8_t)level > _channelActivity[e.channel]) _channelActivity[e.channel] = (uint8_t)level;
            _channelLastMs[e.channel] = millis();
        }
        // Evil-twin check runs ahead of the signature lookups and wins
        // over them. "This SSID is beaconing from a second, different-
        // vendor BSSID" is a statement about the *network*, not about
        // whichever radio chip happens to be in this particular box --
        // and a rogue AP built on commodity hardware would otherwise be
        // logged as whatever its OUI matched, or dropped as UNKNOWN,
        // burying the thing actually worth saying. Only beacons carry
        // an SSID (see the promiscuous callback), so this is naturally
        // limited to them.
        DetectionType t = DetectionType::UNKNOWN;
        // Seeded from the type and then overwritten by whichever row
        // actually matched, if that row has its own grade. An OUI hit off a
        // module vendor and an OUI hit off the product's own registration
        // are the same DetectionType and very different claims.
        Confidence conf = Confidence::HIGH_CONF;
        bool matchedBySsid = false;
        // Ahead of everything, including the evil-twin check: a pwnagotchi
        // told us what it is, in its own words, along with how many
        // handshakes it has taken. No inference beats that, and its
        // throwaway SSID must not be fed to the AP tracker as if it were a
        // network somebody might be impersonating.
        bool evilTwin = false;
        if (e.pwnagotchi) {
            t = DetectionType::HACKER;
            conf = Confidence::HIGH_CONF;
        } else if (e.drone) {
            t = DetectionType::DRONE;
            conf = confidenceFor(t);
        } else if ((evilTwin = (e.ssid[0] && noteApBeacon(e.mac, e.ssid, e.encrypted)))) {
            t = DetectionType::EVILTWIN;
        } else {
            // Check OUI first (per DESIGN.md §6.2 precedence); fall back
            // to the SSID prefix (e.g. an Axon/Flock unit in pairing
            // mode, broadcasting from a WiFi module OUI we don't
            // otherwise know) if the OUI itself didn't match anything.
            t = lookupOui(e.mac, &conf);
            if (t == DetectionType::UNKNOWN && e.ssid[0]) {
                t = lookupSsid(e.ssid);
                matchedBySsid = (t != DetectionType::UNKNOWN);
                // The SSID tables have no per-row grade, so an SSID match
                // falls back to what the type is worth.
                if (matchedBySsid) conf = (t == DetectionType::FLOCK || t == DetectionType::AXON) ? Confidence::LOW_CONF : confidenceFor(t);
            }
        }
        const auto researchMatch = (!evilTwin && !e.pwnagotchi && !e.drone) ? Research::matchWifi(e.mac, e.ssid) : Research::Match{};
        const bool qualified = researchMatch.type != DetectionType::UNKNOWN && (researchMatch.bits & (Research::SSID | Research::IMPORTED));
        if (qualified) { t = researchMatch.type; conf = researchMatch.conf; }
        const char* fpv = (!evilTwin && !e.pwnagotchi && !e.drone) ? Field::fpvName(e.ssid) : nullptr;
        if(fpv){t=DetectionType::FPV;conf=Confidence::LOW_CONF;}
        if (t == DetectionType::UNKNOWN) continue;
        // Disabled types (Settings > DETECTION FILTER) dropped here too
        // -- same reasoning as postBle()'s guard: the dedupe/merge loop
        // just below can re-activate and re-count an already-logged
        // device without ever reaching pushLog().
        if (!Settings::typeEnabled(t)) continue;
        Detection d;
        memset(&d, 0, sizeof(d));
        memcpy(d.mac, e.mac, 6);
        d.rssi    = e.rssi;
        d.channel = e.channel;
        d.type    = t;
        d.addressRole = e.role;
        d.evidence = evilTwin ? MatchEvidence::EVIL_TWIN
                   : e.pwnagotchi ? MatchEvidence::PWNAGOTCHI
                   : e.drone ? MatchEvidence::WIFI_REMOTE_ID
                   : matchedBySsid ? MatchEvidence::SSID : MatchEvidence::OUI;
        d.conf    = (t == DetectionType::EVILTWIN) ? confidenceFor(t) : conf;
        // Vendor label: from the SSID-prefix table if that's what
        // matched, otherwise from the OUI table. An evil twin gets
        // neither -- what matters is which network is being
        // impersonated, so the SSID goes in as the name.
        if (t == DetectionType::EVILTWIN) {
            d.vendor = "EvilTwin";
            strncpy(d.name, e.ssid, sizeof(d.name) - 1);
        } else if (e.pwnagotchi) {
            d.vendor = "Pwnagotchi";
            strncpy(d.name, e.ssid, sizeof(d.name) - 1);
        } else if (e.drone) {
            d.vendor = "DroneID";
            strncpy(d.name, e.ssid, sizeof(d.name) - 1);
        } else if (matchedBySsid) {
            const char* name = ssidVendorName(e.ssid);
            if (name) d.vendor = name;
        } else {
            for (uint16_t k = 0; k < kOuiCount; k++) {
                if (e.mac[0] == kOuiTable[k].b[0] &&
                    e.mac[1] == kOuiTable[k].b[1] &&
                    e.mac[2] == kOuiTable[k].b[2]) {
                    d.vendor = kOuiTable[k].name;
                    break;
                }
            }
        }
        if (qualified) { d.vendor = researchMatch.label; d.evidence = MatchEvidence::RESEARCH_COMPOSITE; d.signature = researchMatch.rule; d.evidenceBits = researchMatch.bits; }
        if(fpv){d.vendor=fpv;d.evidence=MatchEvidence::SSID;}

        d.firstSeen = d.lastSeen = millis();
        d.hits   = 1;
        d.active = true;
        recordObservation(d);
    }
}

void DetectionEngine::pushLog(const Detection& incoming) {
    Detection d=incoming;d.locationKey=LocationLabel::currentKey();
    if (!appendLive(d)) return;
    if(d.channel==0 && (uint8_t)d.type<SpamWatch::TYPES)_newBle[(uint8_t)d.type]++;
    // Counted here, on the Bluetooth host task, and written to flash from
    // loop() (see saveLifetime). A flash write stalls both cores for a
    // millisecond and every so often for a sector erase, and two of them
    // per new detection on the task that receives the adverts was what let
    // a bench flood of new trackers back the radio up until the heap went.
    _lifetimeDirty = true;
    Bingo::note(d.type);
    Dex::note(d.type, d.rssi);
    portENTER_CRITICAL(&s_sdMux);
    const uint8_t next = (uint8_t)((_sdQHead + 1) % SD_Q_CAP);
    if (next != _sdQTail) { _sdQ[_sdQHead] = d; _sdQHead = next; }
    else ++_sdDropped;
    portEXIT_CRITICAL(&s_sdMux);
    queueBlackBox(d, false);
}

static portMUX_TYPE s_bbMux = portMUX_INITIALIZER_UNLOCKED;


void DetectionEngine::queueBlackBox(const Detection& d, bool again) {
    ++_storedEvents;
    if (!BlackBox::ready()) return;
    portENTER_CRITICAL(&s_bbMux);
    const uint8_t next = (uint8_t)((_bbQHead + 1) % BB_Q_CAP);
    if (next != _bbQTail) {       // full: a flood loses black box lines, never detections
        BlackBoxQ& q = _bbQ[_bbQHead];
        memcpy(q.mac, d.mac, 6);
        q.type  = d.type;
        q.again = again;
        q.ms    = millis();
        q.locationKey=d.locationKey;
        _bbQHead = next;
    }
    portEXIT_CRITICAL(&s_bbMux);
}

// One a pass, and only once it is a second and a half old: the flash write
// stalls both cores, so a burst is spread over the loop instead of landing
// on one frame.
void DetectionEngine::drainBlackBox(uint32_t now) {
    BlackBoxQ q;
    portENTER_CRITICAL(&s_bbMux);
    const bool have = _bbQTail != _bbQHead && now - _bbQ[_bbQTail].ms >= 1500;
    if (have) { q = _bbQ[_bbQTail]; _bbQTail = (uint8_t)((_bbQTail + 1) % BB_Q_CAP); }
    portEXIT_CRITICAL(&s_bbMux);
    if (!have) return;

    // Six in a burst, then one every three seconds. Measured on the bench:
    // a flood of two hundred adverts a second is two thousand first sights a
    // minute, which wrote a record fifteen times a second and erased a sector
    // every four. The ring only holds eighteen hundred sightings, so nothing
    // real is lost by capping it -- and a crowded festival stops wearing the
    // flash out at a hundred times the rate an ordinary day does.
    static const uint8_t  BURST = 6;
    static const uint32_t EVERY = 3000;
    static uint8_t  tokens = BURST;
    static uint32_t filled = 0;
    if (!filled) filled = now;
    while (tokens < BURST && now - filled >= EVERY) { tokens++; filled += EVERY; }
    if (now - filled > EVERY) filled = now;      // long gap: start the clock here
    if (!tokens) return;
    tokens--;
    for (uint8_t i = 0; i < _logCount; i++) {
        const Detection& d = _log[(_logHead + LOG_CAP - 1 - i) % LOG_CAP];
        if (d.type == q.type && memcmp(d.mac, q.mac, 6) == 0) {
            Detection saved=d;saved.locationKey=q.locationKey;
            BlackBox::noteDetection(saved, q.again);
            return;
        }
    }
    // Gone from the log already -- two hundred newer devices in a second and
    // a half. Nothing left to say about it.
}

// LOG on the console. Here rather than in clock.cpp because the ring is the
// engine's, and g_engine is the only handle on the live one.
void logDump() {
    if (!g_engine) { Serial.println("[log] no engine"); return; }
    const uint8_t n = g_engine->logCount();
    Serial.printf("[log] %u rows, newest first\n", (unsigned)n);
    Serial.println("row,type,mac,rssi,hits,state,when,simulated,address_provenance");
    for (uint8_t i = 0; i < n; i++) {
        const Detection* d = g_engine->logAt(i);
        if (!d) break;
        char when[16];
        if (d->restored) Clock::formatEpochStamp(d->firstSeen, when, sizeof when);
        else             Clock::formatStamp(d->firstSeen, when, sizeof when);
        Serial.printf("%u,%s,%02x:%02x:%02x:%02x:%02x:%02x,%d,%u,%s,%s,%u,%s\n",
                      (unsigned)i, detectionTypeName(d->type),
                      d->mac[0], d->mac[1], d->mac[2], d->mac[3], d->mac[4], d->mac[5],
                      (int)d->rssi, (unsigned)d->hits,
                      d->restored ? "KEPT" : (d->active ? "here" : "gone"), when,Simulation::marked(*d),Simulation::roleName(d->addressRole));
    }
}


// The lifetime tally, to flash: at most once every five seconds while it
// has changed. Five seconds of counting is what a power cut can lose.
void DetectionEngine::saveLifetime(uint32_t now) {
    if (!_lifetimeDirty || now - _lifetimeSavedMs < 5000) return;
    _lifetimeDirty   = false;
    _lifetimeSavedMs = now;
    _prefs.putUInt("total", _lifetimeTotal);
    saveLifetimeByType();
}

void DetectionEngine::expireStale() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < _logCount; i++) {
        uint8_t slot = (_logHead + LOG_CAP - 1 - i) % LOG_CAP;
        // Signed: lastSeen is written from the radio tasks and can land a
        // moment after `now` was read.
        if (_log[slot].active && (int32_t)(now - _log[slot].lastSeen) > (int32_t)STALE_MS) {
            _log[slot].active = false;
            const Detection& gone=_log[slot];
            if(gone.channel==0 && _spam.noteVanish((uint8_t)gone.type,gone.lastSeen-gone.firstSeen,gone.hits,now))
                Serial.printf("[spam] %s flood: short-lived addresses piling up\n",detectionTypeName(gone.type));
            if (_typeCounts[(uint8_t)_log[slot].type] > 0) {
                _typeCounts[(uint8_t)_log[slot].type]--;
            }
        }
    }
}

static_assert((uint8_t)DetectionType::COUNT<=SpamWatch::TYPES,"SpamWatch type capacity");

const Detection* DetectionEngine::logAt(uint8_t idx) const {
    if (idx >= _logCount) return nullptr;
    uint8_t slot = (_logHead + LOG_CAP - 1 - idx) % LOG_CAP;
    return &_log[slot];
}

// Module-level helpers used by main / UI
static char g_macBuf[20];
const char* macFmt(const uint8_t* mac) {
    formatMac(g_macBuf, sizeof(g_macBuf), mac);
    return g_macBuf;
}


// Three attempts, spaced 500ms, then retire with a counted loss. Internal
// history remains the recovery source; no unlimited RAM backlog is possible.
void DetectionEngine::drainSd(uint32_t now){
 if(!_sdRetry.due(now))return;
 Detection event;bool have;
 portENTER_CRITICAL(&s_sdMux);have=_sdQTail!=_sdQHead;if(have)event=_sdQ[_sdQTail];portEXIT_CRITICAL(&s_sdMux);
 if(!have)return;
 bool mounted=_sd.ready();bool ok=mounted&&_sd.logEvent(event);
 if(!_sdRetry.retire(ok,mounted,now))return;
 if(!ok)++_sdDropped;
 portENTER_CRITICAL(&s_sdMux);_sdQTail=(uint8_t)((_sdQTail+1)%SD_Q_CAP);portEXIT_CRITICAL(&s_sdMux);
}

// Shutdown owns the main task after entry. File writes are synchronous and
// every SdLog/Research write closes its File before this routine advances.
void DetectionEngine::beginShutdown() {
    _stopping.store(true);
    startUpdateRadio();
    Detection incoming;
    for (uint8_t i=0;i<16 && _blePending.pop(incoming,millis());i++) recordObservation(incoming);
    processWiFiQ();
    processDeauthQ();
}
bool DetectionEngine::shutdownTick() {
    if (_sdQTail != _sdQHead) {drainSd(millis());return false;}
    BlackBoxQ q;
    bool have;
    portENTER_CRITICAL(&s_bbMux);
    have = _bbQTail != _bbQHead;
    if (have) {q=_bbQ[_bbQTail];_bbQTail=(uint8_t)((_bbQTail+1)%BB_Q_CAP);}
    portEXIT_CRITICAL(&s_bbMux);
    if (have) {
        for (uint8_t i=0;i<_logCount;i++) {
            const Detection& d=_log[(_logHead+LOG_CAP-1-i)%LOG_CAP];
            if(d.type==q.type&&!memcmp(d.mac,q.mac,6)){Detection saved=d;saved.locationKey=q.locationKey;BlackBox::noteDetection(saved,q.again);break;}
        }
        return false;
    }
    saveLifetime(millis()+5001);
    return true;
}
