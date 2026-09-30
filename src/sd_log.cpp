// SquachWatch-CYD — SD log implementation
#include "sd_log.h"
#include "deauth_tracker.h"
#include "research.h"
namespace Research { bool storageSink(const char*, const char*, bool); }
#include <SD.h>
#include <esp_heap_caps.h>
#include "ff.h"
#include "diskio_impl.h"
#include "diskio.h"
#include <esp_heap_caps.h>
#include "csv_text.h"
#include <stdio.h>
// The Phantoms define CYD (they ARE a CYD) but still need this reference,
// because their touch shares the display's bus and SdLog::begin() has to hand
// SD the display's own SPI instance -- see the comment on that branch below.
#if !defined(CYD) || defined(RLPHANTOM) || defined(RLPHANTOM_R)
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>
// The single TFT_eSPI instance main.cpp already owns and has already
// init()'d by the time SdLog::begin() runs (see the comment below for
// why AWOK/cyd35 specifically need this reference).
extern TFT_eSPI tft;
#endif

// CYD SD card CS — see docs/PINOUT.md. AWOK: CS=14 on the on-board
// slot, sharing the DISPLAY'S VSPI bus (18/23/19). GPIO5 on this board
// is TFT_RST — reusing the CYD's CS=5 would fight the display. cyd35
// shares its display's VSPI bus too (14/13/12, not 18/19/23) but its
// real SD-slot CS is unconfirmed -- 5 is a placeholder guess (SD has
// failed to mount on every real unit tested so far regardless).
#if defined(AWOK)
    #define SD_CS_PIN 14
#else
    #define SD_CS_PIN 5
#endif

// Room for two open files, not the library's default five. The FAT driver
// reserves a 4 KB sector buffer per file slot up front, so five slots want a
// 25 KB block -- more than is left once both radios are up (largest block
// measured at 18 KB on the RL Phantom). This log has one file open at a time,
// plus a directory handle while it prunes old days.
static const uint8_t SD_MAX_FILES = 2;

bool SdLog::begin() {
    if (_ready) return true;
    _memoryLimited = false;
#if defined(TWATCH_S3)
    return false;   // no card slot; GPIO19/20 are the S3's USB pins
#endif
    ff_diskio_get_drive(&_drive); // SD.begin reserves the next free FAT drive below.
    Serial.printf("[sd] mounting: heap %lu, largest block %lu\n", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT), (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
#if defined(CYD35)
    // (The RL Phantom used to land here too, and its SD card never worked as
    // a result: the card is on 18/19/23, and the display's SPI engine never
    // clocked those pins. Its display now runs on HSPI -- see its user setup
    // -- so it takes the original board's branch below with a bus of its own.)
    // The RL Phantom landed here for the same reason cyd35 does, and it cost a
    // tester an evening: its resistive touch chip sits on the DISPLAY's bus, so
    // the extra pins this attaches corrupt MISO for every touch read afterwards.
    // The symptom is precise and was reported exactly as described below --
    // the 4-corner calibration works (it runs BEFORE engine.init() brings SD up)
    // and touch is dead on the very next screen. Not a bad calibration blob: a
    // corrupted bus underneath a perfectly good one.
    //
    // SD.begin(csPin) defaults its SPIClass& parameter to the Arduino
    // *global* `SPI` object -- a separate, never-begun C++ instance
    // from TFT_eSPI's own internal one, even though both ultimately
    // target the same VSPI hardware. SDFS::begin() (ESP32 core's
    // SD.cpp) unconditionally calls that object's own spi.begin() with
    // NO arguments; for a never-begun SPIClass, SPIClass::begin() falls
    // back to the compiled-in esp32dev board defaults -- SCK=18,
    // MISO=19, MOSI=23 -- regardless of this board's real shared-bus
    // pins (14/13/12 here). Root-caused on real cyd35 hardware: those
    // extra pins get ADDITIONALLY attached to VSPI's signals via the
    // GPIO matrix (spiAttachSCK() etc. are additive, not exclusive),
    // corrupting MISO for every touch read afterward even though the
    // display's write-only path looked completely fine.
    //
    // Passing TFT_eSPI's own already-init()'d SPI instance instead
    // makes SDFS::begin()'s internal spi.begin() call a genuine no-op
    // (SPIClass::begin() returns immediately if already begun -- see
    // its own guard), so nothing extra ever gets attached to the bus.
    //
    // AWOK deliberately does NOT get this treatment despite sharing
    // the same VSPI-bus shape: real hardware regression testing showed
    // its touch stops responding once SD.begin() runs with the shared
    // instance passed in (SD.begin() still attempts real transactions
    // over that peripheral even though `begin()` itself becomes a
    // no-op, and AWOK's touch chip is apparently more sensitive to
    // that than cyd35's) -- so it keeps the plain no-args SD.begin()
    // below, same as before this fix existed.
    if (!SD.begin(SD_CS_PIN, tft.getSPIinstance(), 4000000, "/sd", SD_MAX_FILES)) {
#elif defined(AWOK)
    if (!SD.begin(SD_CS_PIN, SPI, 4000000, "/sd", SD_MAX_FILES)) {
#else
    // Original board only: a genuinely separate, dedicated SD bus (not
    // shared with the display), so it does need its own explicit begin()
    // -- SD.begin()'s internal default-pin fallback happens to match
    // this board's real wiring too, but stay explicit for clarity.
    SPI.begin(18, 19, 23, SD_CS_PIN);  // SCK, MISO, MOSI, CS
    if (!SD.begin(SD_CS_PIN, SPI, 4000000, "/sd", SD_MAX_FILES)) {
#endif
        // Said out loud either way: a board with no card, or a card on the
        // wrong pins, ran exactly like one that was logging, and the only
        // way to tell was to pull the card and look.
        Serial.println("[sd] no card, or it did not answer: nothing will be logged");
        _ready = false;
        return false;
    }
    Serial.printf("[sd] card mounted: %llu MB\n", (unsigned long long)(SD.cardSize() >> 20));
    Serial.printf("[sd] mounted: byte heap %lu, largest %lu\n", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT), (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    // Refuse a mount that would leave radio tasks unable to allocate. Do not
    // mark it usable or format anything; the caller can report SD unavailable.
    if (heap_caps_get_free_size(MALLOC_CAP_8BIT) < 12288 ||
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 8192) {
        Serial.println("[sd] insufficient working RAM; unmounting card to keep scanning safe");
        SD.end(); _ready=false; _memoryLimited=true; return false;
    }
    _ready = true;
    Research::setSink(Research::storageSink);
    Research::setReportSink(Research::storageReport);
    openDaily();
    return true;
}

void SdLog::logPressure(uint32_t alerts, uint32_t ble) {
    if (!_ready || (alerts == _loggedAlerts && ble == _loggedBle) || millis() - _pressureAt < 10000) return;
    _pressureAt = millis();
    // Two bounded files, no recursive attempt to log a logging failure.
    File f = SD.open("/dnsp-health.log", FILE_APPEND);
    if (!f) { if (_writeErrors != UINT32_MAX) ++_writeErrors; return; }
    if (f.size() >= 32768) {
        f.close();
        SD.remove("/dnsp-health.old");
        if (!SD.rename("/dnsp-health.log", "/dnsp-health.old")) {
            if (_writeErrors != UINT32_MAX) ++_writeErrors;
            return;
        }
        f = SD.open("/dnsp-health.log", FILE_APPEND);
        if (!f) { if (_writeErrors != UINT32_MAX) ++_writeErrors; return; }
    }
    char line[128];
    snprintf(line, sizeof line, "uptime_ms=%lu alert_queue_omitted=%lu ble_queue_omitted=%lu write_errors=%lu\n",
             (unsigned long)millis(), (unsigned long)alerts, (unsigned long)ble, (unsigned long)_writeErrors);
    if (f.print(line) == strlen(line)) { _loggedAlerts = alerts; _loggedBle = ble; }
    else if (_writeErrors != UINT32_MAX) ++_writeErrors;
    f.close();
}

void SdLog::describe(char* out, size_t cap) {
    if (!_ready && _memoryLimited) {
        snprintf(out, cap, "microSD unmounted: insufficient working RAM after mounting. Scanning continues; SD logging and backups are unavailable. Restart and capture the startup serial log if this repeats.");
        return;
    }
    if (!_ready) {
        snprintf(out, cap, "No mounted microSD card. Insert a FAT-formatted card with power off, then restart. Logging is unavailable.");
        return;
    }
    FATFS* fs = nullptr;
    DWORD freeClusters = 0;
    char drive[] = {char('0' + _drive), ':', 0};
    if (_drive > 9 || f_getfree(drive, &freeClusters, &fs) != FR_OK || !fs || fs->n_fatent < 2) {
        snprintf(out, cap, "Card mounted at boot, but storage information cannot be read now. Power off before checking the card.");
        return;
    }
    const char* format = fs->fs_type == FS_FAT12 ? "FAT12" : fs->fs_type == FS_FAT16 ? "FAT16"
                       : fs->fs_type == FS_FAT32 ? "FAT32" : "unknown";
    const uint64_t total = SD.totalBytes(), used = SD.usedBytes();
    snprintf(out, cap,
        "microSD: %s. Format: %s. Card: %llu MiB. Volume: %llu MiB. Used: %llu MiB (%u%%). Write errors: %lu. Volume name: unavailable in this driver.",
        SD.cardType() == CARD_SDHC ? "SDHC/SDXC" : "SD", format,
        (unsigned long long)(SD.cardSize() >> 20), (unsigned long long)(total >> 20),
        (unsigned long long)(used >> 20), total ? unsigned(used * 100 / total) : 0,
        (unsigned long)_writeErrors);
}

void SdLog::openDaily() {
    if (!_ready) return;
    uint32_t t = millis();
    uint32_t day = t / (24UL * 60UL * 60UL * 1000UL);
    snprintf(_filename, sizeof(_filename), "/squachwatch-%lu.log", (unsigned long)day);
}

void SdLog::logEvent(const Detection& d) {
    if (!_ready) return;
    File f = SD.open(_filename, FILE_APPEND);
    if (!f) { if (_writeErrors != UINT32_MAX) ++_writeErrors; return; }
    char line[208];
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
    // Sanitize any commas in vendor / name
    char vendorSafe[12], nameSafe[20];
    safeCsvText(vendorSafe, sizeof vendorSafe, vendorText(d), strlen(vendorText(d)));
    safeCsvText(nameSafe, sizeof nameSafe, d.name, sizeof d.name);
    if (d.type == DetectionType::DEAUTH) {
        const unsigned targets = d.evidenceBits & DEAUTH_META_TARGET_MASK;
        const unsigned long duration = (unsigned long)(d.lastSeen - d.firstSeen);
        char reason[16];
        if (d.evidenceBits & DEAUTH_META_REASON_VALID)
            snprintf(reason, sizeof reason, "%u", (unsigned)d.signature);
        else
            snprintf(reason, sizeof reason, "unknown");
        snprintf(line, sizeof(line),
                 "%lu,%s,%d,%s,%u,%s,%s,count=%u,window_ms=%lu,targets=%u,reason=%s,protected=%u,unprotected=%u\n",
                 (unsigned long)millis(), detectionTypeName(d.type), d.rssi, mac,
                 d.channel, vendorSafe, nameSafe, (unsigned)d.hits, duration, targets,
                 reason,
                 (d.evidenceBits & DEAUTH_META_PROTECTED_SEEN) ? 1u : 0u,
                 (d.evidenceBits & DEAUTH_META_UNPROTECTED_SEEN) ? 1u : 0u);
    } else {
        snprintf(line, sizeof(line),
                 "%lu,%s,%d,%s,%u,%s,%s\n",
                 (unsigned long)millis(), detectionTypeName(d.type), d.rssi, mac,
                 d.channel, vendorSafe, nameSafe);
    }
    if (f.print(line) != strlen(line) && _writeErrors != UINT32_MAX) ++_writeErrors;
    f.close();
}

void SdLog::wipe() {
    Research::storageWipe();
    if (!_ready) return;
    SD.remove("/dnsp-telemetry.txt");
    SD.remove("/dnsp-fpv-pit.csv");
    SD.remove("/dnsp-health.log");
    SD.remove("/dnsp-health.old");
    // Walk the root and remove every file this firmware writes. Names are
    // /squachwatch-YYYYMMDD.log; matching on the prefix takes them all rather
    // than only today's, which is the whole point of a wipe.
    File dir = SD.open("/");
    if (!dir) return;
    // Collect first, then remove: deleting while iterating openNextFile() is
    // not something the FAT driver promises to survive.
    char victims[16][32];
    int  n = 0;
    for (File f = dir.openNextFile(); f && n < 16; f = dir.openNextFile()) {
        const char* nm = f.name();
        // name() is with or without a leading slash depending on core version;
        // match the basename either way.
        const char* base = nm;
        for (const char* p = nm; *p; p++) if (*p == '/') base = p + 1;
        if (strncmp(base, "squachwatch-", 12) == 0) {
            snprintf(victims[n], sizeof victims[n], "/%s", base);
            n++;
        }
        f.close();
    }
    dir.close();
    for (int i = 0; i < n; i++) SD.remove(victims[i]);
    _filename[0] = '\0';       // force a fresh openDaily() on the next event
}

void SdLog::tick() {
    if (!_ready) return;
    uint32_t now = millis();
    if (now - _lastFlush > 5000) {
        _lastFlush = now;
        // Reopen daily file once an hour (or on day change)
        static uint32_t lastDayCheck = 0;
        if (now - lastDayCheck > 3600000) {
            lastDayCheck = now;
            openDaily();
        }
    }
}


bool SdLog::safeEnd() {
    if (!_ready) return true;
    const bool ok = _drive < 10 && disk_ioctl(_drive, CTRL_SYNC, nullptr) == RES_OK && _writeErrors == 0;
    SD.end();
    _ready = false;
    return ok;
}

bool SdLog::recoveryRemount() {
    if (_ready) safeEnd();
    _drive = 255;
    const bool ok = begin();
    snprintf(_recovery, sizeof _recovery, "%s", ok
        ? "Card found and remounted. Storage services are available."
        : "Card could not be mounted. Power off, reseat it, and try again.");
    return ok;
}

bool SdLog::recoveryTest() {
    if (!_ready && !recoveryRemount()) return false;
    const char* path = "/.dnsp-card-test.tmp";
    static const char sample[] = "DNSP microSD read/write test v1\n";
    SD.remove(path);
    File f = SD.open(path, FILE_WRITE);
    if (!f) { snprintf(_recovery,sizeof _recovery,"Test failed while creating a temporary file."); return false; }
    const bool wrote = f.write((const uint8_t*)sample, sizeof(sample)-1) == sizeof(sample)-1;
    f.flush(); f.close();
    char back[sizeof sample] = {};
    f = SD.open(path, FILE_READ);
    const bool read = f && f.size() == sizeof(sample)-1 && f.read((uint8_t*)back,sizeof(sample)-1) == sizeof(sample)-1;
    f.close();
    const bool same = wrote && read && memcmp(back,sample,sizeof(sample)-1)==0;
    const bool removed = SD.remove(path);
    if (same && removed) snprintf(_recovery,sizeof _recovery,"Read/write test passed; temporary file removed.");
    else snprintf(_recovery,sizeof _recovery,"Card test failed: write %s, read-back %s, cleanup %s.",wrote?"ok":"failed",same?"ok":"failed",removed?"ok":"failed");
    return same && removed;
}

bool SdLog::recoveryFormat() {
    if (!_ready && !recoveryRemount()) {
        snprintf(_recovery,sizeof _recovery,"Format could not start because the card did not mount.");
        return false;
    }
    if (_drive > 9) { snprintf(_recovery,sizeof _recovery,"Format stopped: filesystem drive could not be identified."); return false; }
    FATFS* fs = nullptr; DWORD freeClusters = 0;
    char drive[] = {char('0' + _drive), ':', 0};
    if (f_getfree(drive,&freeClusters,&fs)!=FR_OK || !fs) {
        snprintf(_recovery,sizeof _recovery,"Format stopped: filesystem could not be opened safely.");
        return false;
    }
    void* work = heap_caps_malloc(4096, MALLOC_CAP_8BIT);
    if (!work) { snprintf(_recovery,sizeof _recovery,"Format stopped: not enough working memory."); return false; }
    // No files are held open by SdLog. Temporarily detach the FatFS object,
    // create a fresh volume, then attach the same object the VFS already owns.
    FRESULT r = f_mount(nullptr,drive,0);
    if (r == FR_OK) r = f_mkfs(drive,FM_ANY,0,work,4096);
    if (r == FR_OK) r = f_mount(fs,drive,1);
    free(work);
    if (r != FR_OK) {
        _ready = false; SD.end();
        snprintf(_recovery,sizeof _recovery,"Format failed (filesystem error %u). Power off before removing the card.",(unsigned)r);
        return false;
    }
    _writeErrors=0;_memoryLimited=false;_filename[0]=0;openDaily();
    const bool ok = recoveryTest();
    if (ok) snprintf(_recovery,sizeof _recovery,"Format complete. FAT volume created and read/write test passed.");
    return ok;
}
