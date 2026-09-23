// SquachWatch-CYD — ASTM F3411 Remote ID decoder. See remote_id.h.
#include "remote_id.h"
#include <string.h>
#include <math.h>

namespace RemoteId {

// ---- the wire format ----------------------------------------------------
// A Bluetooth Legacy advert is a run of AD structures, each one a length
// byte, a type byte, then length-1 bytes of data. Remote ID rides in the
// Service Data structure (type 0x16) under UUID 0xFFFA, and inside that
// sits a one-byte application code, a one-byte message counter, then
// exactly 25 bytes of message.
static const uint8_t AD_SERVICE_DATA_16 = 0x16;
static const uint16_t ODID_UUID = 0xFFFA;
static const uint8_t ODID_APP_CODE = 0x0D;
static const uint8_t MSG_SIZE = 25;

// Message types live in the TOP nibble of byte 0. The bottom nibble is the
// protocol version, which we do not check: the field layouts this reads
// have been stable across every published version, and refusing to decode
// a drone because it advertised version 3 would be worse than decoding it.
static const uint8_t MSG_BASIC_ID = 0x0;
static const uint8_t MSG_LOCATION = 0x1;
static const uint8_t MSG_SYSTEM = 0x4;

// Fixed-point scales, straight out of the reference implementation:
// coordinates are degrees x 10^7, and altitude is half-metres offset by
// 1000m so that below-sea-level values still fit an unsigned 16-bit field.
static const double LATLON_MULT = 10000000.0;
static const float ALT_DIV = 0.5f;
static const float ALT_ADDER = 1000.0f;

// Little-endian, because the standard's structs are packed and every
// device implementing it is little-endian. Read byte by byte rather than
// cast: the payload comes off the radio unaligned, and a 32-bit load at an
// odd address is a LoadStoreError exception on this chip rather than a
// slow read like it would be on a PC.
static int32_t rd32(const uint8_t *p) {
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
                     ((uint32_t)p[3] << 24));
}
static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

const char *uaTypeName(uint8_t t) {
    switch (t) {
    case 1:
        return "PLANE";
    case 2:
        return "QUAD";
    case 3:
        return "HELI";
    case 4:
        return "GYRO";
    case 5:
        return "VTOL";
    case 6:
        return "ORNITH";
    case 7:
        return "GLIDER";
    case 8:
        return "KITE";
    case 9:
        return "FREEBAL";
    case 10:
        return "CAPBAL";
    case 11:
        return "AIRSHIP";
    case 12:
        return "PARA";
    case 13:
        return "ROCKET";
    case 14:
        return "TETHER";
    case 15:
        return "GROUND";
    default:
        return "UNKNOWN";
    }
}

void reset(Info &out) {
    out = Info();
}

// Decodes one 25-byte message body into `out`.
static bool mergeMessage(const uint8_t *m, Info &out, uint32_t now) {
    if ((m[0] & 15) > 2)
        return false; // unsupported layout: do not guess
    const uint8_t type = (uint8_t)((m[0] >> 4) & 0x0F);

    if (type == MSG_BASIC_ID) {
        // Byte 1 splits IDType (high) from UAType (low); the serial is the
        // next 20 bytes, space- or NUL-padded rather than terminated.
        const uint8_t idType = m[1] >> 4;
        char id[21]{};
        for (int i = 0; i < 20; i++)
            id[i] = m[2 + i] >= 32 && m[2 + i] <= 126 ? m[2 + i] : 0;
        for (int i = 19; i >= 0 && (id[i] == ' ' || id[i] == 0); --i)
            id[i] = 0;
        if (out.haveBasic && out.idType == idType && strcmp(out.serial, id) != 0)
            out.quality |= 4;
        out.idType = idType;
        out.basicAt = now;
        out.uaType = (uint8_t)(m[1] & 0x0F);
        memcpy(out.serial, id, 20);
        out.serial[20] = '\0';
        for (int i = 19; i >= 0; i--) {
            if (out.serial[i] == ' ' || out.serial[i] == '\0')
                out.serial[i] = '\0';
            else
                break;
        }
        out.haveBasic = true;
        out.at = now;
        return true;
    }

    if (type == MSG_LOCATION) {
        const int32_t la = rd32(m + 5);
        const int32_t lo = rd32(m + 9);
        // 0,0 is the standard's "unknown", and it is also a real place, so
        // treating it as no-fix is the lesser wrong of the two.
        if (la < -900000000 || la > 900000000 || lo < -1800000000 || lo > 1800000000) {
            out.quality |= 1;
            return true;
        }
        if (la == 0 && lo == 0) {
            out.noFix = true;
            return true;
        }
        const float lat = (float)((double)la / LATLON_MULT),
                    lon = (float)((double)lo / LATLON_MULT);
        if (out.haveLoc && (uint32_t)(now - out.locAt) < 30000) {
            const float dy = (lat - out.lat) * 111320.0f;
            float dl = lon - out.lon;
            if (dl > 180)
                dl -= 360;
            if (dl < -180)
                dl += 360;
            const float dx = dl * 111320.0f * cosf(lat * 0.0174532925f);
            const float allowed = 200.0f + 350.0f * ((uint32_t)(now - out.locAt) / 1000.0f);
            if (dx * dx + dy * dy > allowed * allowed)
                out.quality |= 2;
        }
        const uint16_t stamp = rd16(m + 21);
        if (stamp != 65535 && stamp >= 36000)
            out.quality |= 8;
        if (out.haveLoc && stamp != 65535 && stamp == out.locStamp) {
            if (now - out.stampAt > 15000)
                out.quality |= 16;
        } else
            out.stampAt = now;
        out.locStamp = stamp;
        out.noFix = false;
        out.locAt = now;
        out.lat = (float)((double)la / LATLON_MULT);
        out.lon = (float)((double)lo / LATLON_MULT);
        out.altM = (float)rd16(m + 15) * ALT_DIV - ALT_ADDER;
        out.haveLoc = true;
        out.at = now;
        return true;
    }

    if (type == MSG_SYSTEM) {
        // The interesting one. Bytes 2-9 are where the pilot is standing.
        const int32_t la = rd32(m + 2);
        const int32_t lo = rd32(m + 6);
        if (la < -900000000 || la > 900000000 || lo < -1800000000 || lo > 1800000000) {
            out.quality |= 1;
            return true;
        }
        if (la == 0 && lo == 0)
            return true;
        out.operatorAt = now;
        out.opLat = (float)((double)la / LATLON_MULT);
        out.opLon = (float)((double)lo / LATLON_MULT);
        out.haveOperator = true;
        out.at = now;
        return true;
    }

    // Auth, Self-ID, Operator-ID and Message Pack all reach here. Pack
    // (0xF) is the one worth naming: it wraps several messages at once and
    // is what the WiFi Beacon form uses, but it does not fit in a 31-byte
    // legacy advert, so nothing that arrives on this path can be one.
    return false;
}

bool merge(const uint8_t *payload, uint8_t len, Info &out, uint32_t now) {
    if (!payload || len < 4)
        return false;

    uint8_t i = 0;
    while (i < len) {
        const uint8_t adLen = payload[i];
        // A zero length is the standard's end-of-data padding, and any
        // structure claiming to run past the buffer is malformed. Both mean
        // stop rather than continue: this is attacker-reachable input.
        if (adLen == 0)
            break;
        if ((uint16_t)i + 1u + adLen > (uint16_t)len)
            break;

        const uint8_t adType = payload[i + 1];
        const uint8_t *adData = payload + i + 2;
        const uint8_t adSize = (uint8_t)(adLen - 1); // type byte excluded

        if (adType == AD_SERVICE_DATA_16 && adSize >= 3 && rd16(adData) == ODID_UUID &&
            adData[2] == ODID_APP_CODE) {
            // UUID, app code, counter, then the message itself.
            if (adSize >= 4 + MSG_SIZE) {
                return mergeMessage(adData + 4, out, now);
            }
            return false;
        }
        i = (uint8_t)(i + 1 + adLen);
    }
    return false;
}

bool mergePack(const uint8_t *p, size_t n, Info &out, uint32_t now) {
    if (!p || n < 3 || (p[0] >> 4) != 15 || (p[0] & 15) > 2 || p[1] != 25 || !p[2] || p[2] > 9 ||
        n != 3u + 25u * p[2])
        return false;
    Info copy = out;
    bool any = false;
    for (size_t i = 0; i < p[2]; i++)
        any = mergeMessage(p + 3 + i * 25, copy, now) || any;
    if (any)
        out = copy;
    return any;
}
bool wifiPayload(const uint8_t *f, size_t n, const uint8_t *&data, size_t &size) {
    data = nullptr;
    size = 0;
    if (!f || n < 24 || n > 1024 || ((f[1] & 0x44) || (f[22] & 15)))
        return false;
    if (f[0] == 0x80) {
        if (n < 36)
            return false;
        for (size_t i = 36; i < n;) {
            if (n - i < 2 || f[i + 1] > n - i - 2)
                return false;
            const size_t z = f[i + 1];
            if (f[i] == 221 && z >= 8 && memcmp(f + i + 2, "\xFA\x0B\xBC\x0D", 4) == 0) {
                data = f + i + 7;
                size = z - 5; // OUI+type+counter precede message pack
            }
            i += z + 2;
        }
    } else if (f[0] == 0xD0) {
        const uint8_t hdr[] = {4, 9, 0x50, 0x6f, 0x9a, 0x13};
        const uint8_t dest[] = {0x51, 0x6f, 0x9a, 1, 0, 0};
        const uint8_t service[] = {0x88, 0x69, 0x19, 0x9d, 0x92, 9};
        if (n < 30 || memcmp(f + 24, hdr, 6) || memcmp(f + 4, dest, 6))
            return false;
        for (size_t i = 30; i < n;) {
            if (n - i < 3)
                return false;
            const size_t z = rd16(f + i + 1);
            if (z > n - i - 3)
                return false;
            const uint8_t *a = f + i + 3;
            // Publish service descriptor with service-info only; other forms unsupported.
            if (f[i] == 3 && z >= 11 && !memcmp(a, service, 6) && a[8] == 0x10) {
                size_t count = a[9];
                if (count < 4 || count != z - 10)
                    return false;
                data = a + 11;
                size = count - 1;
            }
            i += z + 3;
        }
    }
    if (!data)
        return false;
    Info scratch;
    return mergePack(data, size, scratch, 0);
}
const char *qualityText(const Info &i, uint32_t now) {
    if (i.quality & 1)
        return "Invalid coordinates received";
    if (i.quality & 4)
        return "Identity changed; verify observation";
    if (i.quality & 2)
        return "Unusual position jump; verify reading";
    if (i.quality & 8)
        return "Invalid location timestamp";
    if (i.quality & 16)
        return "Location timestamp stopped advancing";
    if (i.quality & 32)
        return "Identity seen from multiple sources";
    if (i.noFix)
        return "No current fix; last position retained";
    if (i.haveLoc && (uint32_t)(now - i.locAt) > 15000)
        return "Position stale: more than 15 seconds";
    return "Broadcast claims; identity not authenticated";
}
} // namespace RemoteId
