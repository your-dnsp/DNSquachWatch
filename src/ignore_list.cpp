// SquachWatch-CYD — per-device alert suppression ("IGNORE") implementation
#include "ignore_list.h"
#include <Preferences.h>
#include <Arduino.h>
#include <string.h>

namespace IgnoreList {
// Parallel arrays avoid per-record alignment padding: 320 bytes, bounded.
static uint8_t s_snz[SNOOZE_MAX][6];
static uint32_t s_snzDeadline[SNOOZE_MAX];
static uint8_t s_snzN = 0;


static const uint8_t REC = 7;      // mac[6] + DetectionType

static Preferences s_prefs;
static bool        s_loaded = false;
static uint8_t     s_rec[MAX * REC];
static uint8_t     s_count = 0;

static const char* NS      = "ignore";
static const char* KEY     = "dev";    // 7-byte records
static const char* KEY_OLD = "macs";   // pre-type format, 6-byte MACs

static void save() {
    if (s_count == 0) {
        // Preferences::putBytes returns early on a zero-length value without
        // touching NVS, so saving an empty list is a silent no-op and the old
        // blob survives. Removing the last device therefore left it on disk
        // and it came back on the next boot. Emptying the list has to delete
        // the key instead.
        s_prefs.remove(KEY);
        return;
    }
    s_prefs.putBytes(KEY, s_rec, (size_t)s_count * REC);
}

void begin() {
    if (s_loaded) return;
    s_prefs.begin(NS, false);
    // getBytesLength on a missing key is 0, which is exactly the empty
    // list -- no separate "has it ever been written" flag needed.
    size_t len = s_prefs.getBytesLength(KEY);
    if (len > sizeof(s_rec)) len = sizeof(s_rec);
    if (len >= REC) {
        s_prefs.getBytes(KEY, s_rec, len);
        s_count = (uint8_t)(len / REC);
        s_loaded = true;
        return;
    }

    // Nothing in the new key: migrate anything left in the old one, giving
    // those entries UNKNOWN. A separate key rather than sniffing the blob
    // length, because 6- and 7-byte records share lengths at 42, 84 and 126 --
    // length alone cannot tell the two formats apart, and guessing wrong would
    // shred somebody's list.
    size_t old = s_prefs.getBytesLength(KEY_OLD);
    s_count = 0;
    if (old >= 6) {
        uint8_t tmp[MAX * 6];
        if (old > sizeof(tmp)) old = sizeof(tmp);
        s_prefs.getBytes(KEY_OLD, tmp, old);
        s_count = (uint8_t)(old / 6u);
        for (uint8_t i = 0; i < s_count; i++) {
            memcpy(&s_rec[(size_t)i * REC], &tmp[(size_t)i * 6u], 6);
            s_rec[(size_t)i * REC + 6] = (uint8_t)DetectionType::UNKNOWN;
        }
        save();
        s_prefs.remove(KEY_OLD);
    }
    s_loaded = true;
}

static int indexOf(const uint8_t* mac) {
    if (!mac) return -1;
    for (uint8_t i = 0; i < s_count; i++)
        if (memcmp(&s_rec[(size_t)i * REC], mac, 6) == 0) return (int)i;
    // BLE addresses saved before v1.25.0 used NimBLE's internal byte order.
    // Preserve those ignore entries while new detections use printed order.
    for (uint8_t i = 0; i < s_count; i++) {
        const uint8_t* old = &s_rec[(size_t)i * REC];
        bool same = true;
        for (uint8_t b = 0; b < 6 && same; b++) same = old[b] == mac[5 - b];
        if (same) return (int)i;
    }
    return -1;
}

bool contains(const uint8_t* mac) {
    begin();
    return indexOf(mac) >= 0;
}

bool add(const uint8_t* mac, DetectionType type) {
    begin();
    if (!mac || s_count >= MAX) return false;
    if (indexOf(mac) >= 0) return false;
    memcpy(&s_rec[(size_t)s_count * REC], mac, 6);
    s_rec[(size_t)s_count * REC + 6] = (uint8_t)type;
    s_count++;
    save();
    return true;
}

bool remove(const uint8_t* mac) {
    begin();
    const int idx = indexOf(mac);
    if (idx < 0) return false;
    // Order carries no meaning here, so the last entry backfills the hole
    // rather than shifting the tail down.
    const uint8_t last = (uint8_t)(s_count - 1);
    if ((uint8_t)idx != last)
        memcpy(&s_rec[(size_t)idx * REC], &s_rec[(size_t)last * REC], REC);
    s_count--;
    save();
    return true;
}

uint8_t count() { begin(); return s_count; }

const uint8_t* macAt(uint8_t idx) {
    begin();
    if (idx >= s_count) return nullptr;
    return &s_rec[(size_t)idx * REC];
}

DetectionType typeAt(uint8_t idx) {
    begin();
    if (idx >= s_count) return DetectionType::UNKNOWN;
    const uint8_t v = s_rec[(size_t)idx * REC + 6];
    return (v < (uint8_t)DetectionType::COUNT) ? (DetectionType)v
                                               : DetectionType::UNKNOWN;
}

void clear() {
    begin();
    s_count = 0;
    s_snzN = 0;
    s_prefs.remove(KEY);
}

// Timed snoozes never write NVS. Full tables report failure rather than silently
// replacing another device's active timer. Expired slots are reclaimed first.
static void eraseSnooze(uint8_t i) {
    --s_snzN;
    if (i != s_snzN) {
        memcpy(s_snz[i], s_snz[s_snzN], 6);
        s_snzDeadline[i] = s_snzDeadline[s_snzN];
    }
}
static void pruneSnoozes(uint32_t now) {
    for (uint8_t i = 0; i < s_snzN; ) {
        // A live deadline can be at most four hours ahead, including across
        // millis() rollover. Also reject long-stale entries after half a cycle.
        const uint32_t left = s_snzDeadline[i] - now;
        if (!left || left > 14400000UL) eraseSnooze(i);
        else ++i;
    }
}
static int snoozeIndex(const uint8_t* mac) {
    if (mac) for (uint8_t i = 0; i < s_snzN; ++i)
        if (!memcmp(s_snz[i],mac,6)) return i;
    return -1;
}
uint32_t snoozeRemaining(const uint8_t* mac) {
    const uint32_t now = millis();pruneSnoozes(now);
    const int i = snoozeIndex(mac);
    return i < 0 ? 0 : (uint32_t)(s_snzDeadline[i] - now);
}
bool snoozed(const uint8_t* mac) { return snoozeRemaining(mac) != 0; }
bool snooze(const uint8_t* mac, SnoozeDuration duration) {
    if (!mac || (uint8_t)duration > (uint8_t)SnoozeDuration::HOUR_4) return false;
    static const uint32_t MS[] = {600000,1800000,3600000,14400000};
    const uint32_t now = millis();pruneSnoozes(now);
    int i = snoozeIndex(mac);
    if (i < 0) {
        if (s_snzN == SNOOZE_MAX) return false;
        i = s_snzN++;memcpy(s_snz[i],mac,6);
    }
    s_snzDeadline[i] = now + MS[(uint8_t)duration];
    return true;
}
void snooze(const uint8_t* mac) { (void)snooze(mac,SnoozeDuration::MIN_10); }
bool unsnooze(const uint8_t* mac) {
    pruneSnoozes(millis());const int i = snoozeIndex(mac);
    if (i < 0) return false;
    eraseSnooze((uint8_t)i);return true;
}
bool silenced(const uint8_t* mac) { return contains(mac) || snoozed(mac); }

} // namespace IgnoreList
