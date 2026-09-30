#include "deauth_tracker.h"

uint16_t DeauthBurstTracker::macHash(const uint8_t mac[6]) {
    uint16_t h = 0x5A6Du;
    for (uint8_t i = 0; i < 6; ++i) h = (uint16_t)((h << 5) ^ (h >> 11) ^ mac[i]);
    return h;
}

DeauthBurstTracker::Entry& DeauthBurstTracker::entryFor(const uint8_t source[6], uint32_t now) {
    for (uint8_t i = 0; i < TABLE_CAP; ++i)
        if (entries_[i].used && memcmp(entries_[i].source, source, 6) == 0) return entries_[i];
    for (uint8_t i = 0; i < TABLE_CAP; ++i) {
        if (!entries_[i].used) {
            memset(&entries_[i], 0, sizeof entries_[i]);
            entries_[i].used = true;
            memcpy(entries_[i].source, source, 6);
            return entries_[i];
        }
    }
    uint8_t oldest = 0;
    uint32_t oldestAge = now - entries_[0].lastSeen;
    for (uint8_t i = 1; i < TABLE_CAP; ++i) {
        const uint32_t age = now - entries_[i].lastSeen; // wrap-safe unsigned age
        if (age > oldestAge) { oldest = i; oldestAge = age; }
    }
    memset(&entries_[oldest], 0, sizeof entries_[oldest]);
    entries_[oldest].used = true;
    memcpy(entries_[oldest].source, source, 6);
    return entries_[oldest];
}

DeauthBurstResult DeauthBurstTracker::note(const DeauthFrameEvidence& frame, uint32_t now) {
    DeauthBurstResult out{};
    Entry& e = entryFor(frame.source, now);

    // A lone deauthentication is ordinary traffic. Only a coherent burst
    // from this same claimed transmitter is eligible to alert.
    if (e.samples && (uint32_t)(now - e.lastSeen) > WINDOW_MS) {
        e.head = 0;
        e.samples = 0;
    }

    const uint8_t slot = e.head;
    e.at[slot] = now;
    e.targetHash[slot] = macHash(frame.destination);
    memcpy(e.bssid[slot], frame.bssid, 6);
    e.reason[slot] = frame.reason;
    e.flags[slot] = (frame.reasonValid ? 0x01 : 0) | (frame.protectedFrame ? 0x02 : 0);
    e.head = (uint8_t)((e.head + 1) % THRESHOLD);
    if (e.samples < THRESHOLD) ++e.samples;
    e.lastSeen = now;
    e.rssi = frame.rssi;
    e.channel = frame.channel;

    if (e.samples < THRESHOLD) return out;

    uint8_t recent[THRESHOLD];
    uint8_t n = 0;
    for (uint8_t i = 0; i < e.samples; ++i) {
        if ((uint32_t)(now - e.at[i]) <= WINDOW_MS) recent[n++] = i;
    }
    if (n < THRESHOLD) return out;
    if (e.fired && (uint32_t)(now - e.lastFire) < COOLDOWN_MS) return out;

    uint32_t oldestAge = 0;
    uint16_t targets[THRESHOLD];
    uint8_t targetCount = 0;
    bool sameBssid = true;
    bool protectedSeen = false, unprotectedSeen = false;
    bool reasonValid = false;
    uint16_t latestReason = 0;
    uint32_t latestReasonAge = UINT32_MAX;
    const uint8_t firstSlot = recent[0];
    for (uint8_t k = 0; k < n; ++k) {
        const uint8_t i = recent[k];
        const uint32_t age = now - e.at[i];
        if (age > oldestAge) oldestAge = age;
        if (memcmp(e.bssid[firstSlot], e.bssid[i], 6) != 0) sameBssid = false;
        bool known = false;
        for (uint8_t j = 0; j < targetCount; ++j) if (targets[j] == e.targetHash[i]) known = true;
        if (!known) targets[targetCount++] = e.targetHash[i];
        if (e.flags[i] & 0x02) protectedSeen = true; else unprotectedSeen = true;
        if ((e.flags[i] & 0x01) && age <= latestReasonAge) {
            latestReasonAge = age;
            latestReason = e.reason[i];
            reasonValid = true;
        }
    }

    e.lastFire = now;
    e.fired = true;
    out.alert = true;
    memcpy(out.source, e.source, 6);
    if (sameBssid) memcpy(out.bssid, e.bssid[firstSlot], 6);
    out.rssi = e.rssi;
    out.channel = e.channel;
    out.count = n;
    out.distinctTargets = targetCount;
    out.firstMs = now - oldestAge;
    out.lastMs = now;
    out.reason = latestReason;
    out.reasonValid = reasonValid;
    out.protectedSeen = protectedSeen;
    out.unprotectedSeen = unprotectedSeen;
    out.sameBssid = sameBssid;
    return out;
}
