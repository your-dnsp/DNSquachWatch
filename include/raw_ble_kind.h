#pragma once
#include <stddef.h>
#include <stdint.h>

// What a raw-scan device's advert says it is, when it gives no name (phones
// almost never do: iOS keeps "Dave's iPhone" off the air). A best guess from
// who made the advert and what it is for, so a phone can be picked out of the
// list and HUNTed. Ranked: a later advert only ever raises the guess.
enum RawBleKind : uint8_t {
    RAW_KIND_NONE = 0,
    RAW_KIND_APPLE,      // Apple, nothing more specific
    RAW_KIND_SAMSUNG,    // Samsung's company ID: Galaxy phones, also Buds, tags, TVs
    RAW_KIND_AIRPODS,    // Apple proximity pairing (0x07)
    RAW_KIND_ANDROID,    // Google Nearby / Quick Share (service 0xFEF3)
    RAW_KIND_IPHONE,     // Apple Nearby Info / Handoff (0x10, 0x0C): an iPhone, or an iPad, Mac or Watch in use
};
// In brackets, so Privacy Mode leaves it alone like "(unnamed)": it is ours, not theirs.
inline const char* rawBleKindLabel(uint8_t k) {
    switch (k) {
        case RAW_KIND_IPHONE:  return "(iPhone?)";
        case RAW_KIND_ANDROID: return "(Android?)";
        case RAW_KIND_AIRPODS: return "(AirPods?)";
        case RAW_KIND_SAMSUNG: return "(Samsung?)";
        case RAW_KIND_APPLE:   return "(Apple)";
        default:               return "(unnamed)";
    }
}
// The guess from an advert's raw bytes (length-type-data fields).
inline uint8_t rawBleKindOf(const uint8_t* p, size_t n) {
    if (!p) return RAW_KIND_NONE;
    uint8_t k = RAW_KIND_NONE;
    auto raise = [&](uint8_t v) { if (v > k) k = v; };
    for (size_t i = 0; i + 1 < n; ) {
        const uint8_t len = p[i];
        if (!len || len > n - i - 1) break;
        const uint8_t type = p[i + 1];
        const uint8_t* d = p + i + 2;
        const size_t dn = len - 1;
        if (type == 0xFF && dn >= 2) {
            const uint16_t cid = (uint16_t)(d[0] | (d[1] << 8));
            if (cid == 0x004C) {
                raise(RAW_KIND_APPLE);
                for (size_t j = 2; j + 1 < dn; ) {          // Continuity: type, length, data
                    const uint8_t t = d[j], l = d[j + 1];
                    if (l > dn - j - 2) break;
                    if (t == 0x10 || t == 0x0C) raise(RAW_KIND_IPHONE);
                    else if (t == 0x07)         raise(RAW_KIND_AIRPODS);
                    j += 2 + l;
                }
            } else if (cid == 0x0075) raise(RAW_KIND_SAMSUNG);
        } else if ((type == 0x16 && dn >= 2) || type == 0x02 || type == 0x03) {
            // Service data (its UUID first), or a list of 16-bit service UUIDs.
            for (size_t j = 0; j + 1 < dn; j += 2) {
                if ((uint16_t)(d[j] | (d[j + 1] << 8)) == 0xFEF3) raise(RAW_KIND_ANDROID);
                if (type == 0x16) break;
            }
        }
        i += 1 + len;
    }
    return k;
}
