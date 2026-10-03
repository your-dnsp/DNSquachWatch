#pragma once
#include "state.h"
#include <cstring>

// A shared observation merger for WiFi and Bluetooth. Returns whether this
// is a new encounter or stronger evidence deserving another notification.
inline bool mergeObservation(Detection& row, const Detection& seen, uint32_t now) {
    const bool returning = !row.active;
    const bool stronger = seen.conf > row.conf;
    if (seen.name[0]) {
        memcpy(row.name, seen.name, sizeof row.name);
        row.name[sizeof row.name - 1] = 0;
    }
    if (stronger || (seen.conf == row.conf && seen.evidence == MatchEvidence::RESEARCH_COMPOSITE) || (row.evidence == MatchEvidence::UNKNOWN && seen.conf >= row.conf)) {
        row.conf = seen.conf;
        row.evidence = seen.evidence;
        row.signature = seen.signature;
        row.evidenceBits = seen.evidenceBits;
        row.vendor = seen.vendor;
        row.addressRole = seen.addressRole;
    } else if (!row.vendor) row.vendor = seen.vendor;
    const uint8_t at = (uint8_t)(now >> 11);
    if (row.prevAt != at) { row.prevRssi = row.rssi; row.prevAt = at; }
    row.rssi = seen.rssi;
    row.channel = seen.channel;
    row.lastSeen = now;
    if (returning) {
        if (row.hits < UINT16_MAX) ++row.hits;
        row.firstSeen = now;
        row.active = true;
        row.restored = 0;
    }
    return returning || stronger;
}
