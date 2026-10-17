#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Evidence copied from a deauthentication/disassociation management frame. Address 2 is
// only the transmitter address claimed by the frame; management addresses
// are unauthenticated on many networks and can be spoofed.
struct DeauthFrameEvidence {
    uint8_t source[6];
    uint8_t destination[6];
    uint8_t bssid[6];
    int8_t  rssi;
    uint8_t channel;
    uint16_t reason;
    bool reasonValid;
    bool protectedFrame;
};

// Parse only the fixed fields needed by the detector. A standard management
// header is 24 bytes; the two-byte deauthentication reason follows it.
inline bool parseDeauthFrame(const uint8_t* frame, size_t len, int8_t rssi,
                             uint8_t channel, DeauthFrameEvidence& out) {
    if (!frame || len < 24) return false;
    const uint8_t type = (frame[0] & 0x0C) >> 2;
    const uint8_t subtype = (frame[0] & 0xF0) >> 4;
    if (type != 0 || (subtype != 12 && subtype != 10)) return false;
    memcpy(out.destination, frame + 4, 6);   // Address 1
    memcpy(out.source,      frame + 10, 6);  // Address 2 (claimed transmitter)
    memcpy(out.bssid,       frame + 16, 6);  // Address 3
    out.rssi = rssi;
    out.channel = channel;
    out.protectedFrame = (frame[1] & 0x40) != 0;
    // A protected robust-management frame encrypts/authenticates its body;
    // bytes 24..25 must not be presented as a plaintext reason code.
    out.reasonValid = len >= 26 && !out.protectedFrame;
    out.reason = out.reasonValid ? (uint16_t)(frame[24] | ((uint16_t)frame[25] << 8)) : 0;
    return true;
}

// Runtime encoding in Detection::evidenceBits for DEAUTH records. These
// fields are not vendor/OUI evidence and must not be interpreted as such.
static const uint16_t DEAUTH_META_TARGET_MASK      = 0x001F;
static const uint16_t DEAUTH_META_REASON_VALID     = 0x0020;
static const uint16_t DEAUTH_META_PROTECTED_SEEN   = 0x0040;
static const uint16_t DEAUTH_META_UNPROTECTED_SEEN = 0x0080;
static const uint16_t DEAUTH_META_SAME_BSSID       = 0x0100;
static const uint16_t DEAUTH_META_BSSID_VALID      = 0x0200;

struct DeauthBurstResult {
    bool alert;
    uint8_t source[6];
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
    uint8_t count;
    uint8_t distinctTargets;
    uint32_t firstMs;
    uint32_t lastMs;
    uint16_t reason;
    bool reasonValid;
    bool protectedSeen;
    bool unprotectedSeen;
    bool sameBssid;
};

// A bounded per-source detector with a true sliding window: twelve observed
// deauthentication/disassociation frames, or six broadcast-destination frames,
// from one claimed transmitter must fit within WINDOW_MS. Retain twelve samples:
// if an earlier broadcast has been evicted, twelve newer frames already suffice.
// A gap longer than the window clears that source's samples.
//
// The table never allocates. A new source uses an empty slot, otherwise the
// least-recently-seen slot is evicted (lowest index wins a tie).
class DeauthBurstTracker {
public:
    static const uint8_t  TABLE_CAP   = 8;
    static const uint8_t  THRESHOLD   = 12;
    static const uint8_t  BROADCAST_THRESHOLD = 6;
    static const uint32_t WINDOW_MS   = 3000;
    static const uint32_t COOLDOWN_MS = 15000;

    DeauthBurstResult note(const DeauthFrameEvidence& frame, uint32_t now);

    // Exposed for resource accounting and focused host tests.
    struct Entry {
        uint8_t source[6];
        uint8_t bssid[6];
        uint32_t at[THRESHOLD];
        uint16_t targetHash[THRESHOLD]; // bounded approximate distinct count
        uint16_t reason[THRESHOLD];
        uint32_t lastSeen;
        uint32_t lastFire;
        int8_t rssi;
        uint8_t channel;
        uint8_t flags[THRESHOLD]; // bit 0 reason valid, bit 1 protected, bit 2 broadcast
        uint8_t bssidRun; // consecutive frames with this exact BSSID, saturates at THRESHOLD
        uint8_t head;
        uint8_t samples;
        bool used;
        bool fired;
    };

    static size_t tableBytes() { return sizeof(Entry) * TABLE_CAP; }

private:
    Entry entries_[TABLE_CAP]{};
    Entry& entryFor(const uint8_t source[6], uint32_t now);
    static uint16_t macHash(const uint8_t mac[6]);
};
