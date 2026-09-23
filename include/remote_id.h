#pragma once
#include <stdint.h>
#include <stddef.h>

namespace RemoteId {

// One aircraft, assembled over several adverts.
//
// A drone does not send all of this at once. Basic ID, Location and System
// are separate messages broadcast in rotation, so this fills in over a
// second or two and each `have` flag says whether that part has arrived
// yet. Anything not yet seen must not be drawn as zero -- zero is a real
// coordinate in the Gulf of Guinea.
struct Info {
    uint32_t basicAt = 0, locAt = 0, operatorAt = 0, stampAt = 0;
    uint16_t locStamp = 65535;
    uint8_t idType = 0, quality = 0; // coordinates/jump/ID/timestamp/replay/duplicate flags
    bool noFix = false;
    bool haveBasic = false;
    bool haveLoc = false;
    bool haveOperator = false;
    char serial[21] = {0}; // ODID_ID_SIZE is 20, plus the terminator
    uint8_t uaType = 0;
    float lat = 0, lon = 0;     // the aircraft, degrees
    float altM = 0;             // geodetic altitude, metres
    float opLat = 0, opLon = 0; // broadcast operator location; may be a takeoff/static point
    uint32_t at = 0;            // millis() of the last message merged in
};

// "QUAD", "PLANE", "HELI"... short enough for a 320px row.
const char *uaTypeName(uint8_t t);

// Pulls one message out of a raw advertisement and merges it into `out`,
// leaving fields the message did not carry alone. Returns true if this
// advert actually contained a Remote ID message.
//
// Merging rather than returning a fresh struct is the whole point: the
// caller keeps one Info per aircraft and feeds every advert through it.
bool merge(const uint8_t *payload, uint8_t len, Info &out, uint32_t now);

bool mergePack(const uint8_t *data, size_t len, Info &out, uint32_t now);
// Raw management frame without FCS. Beacon and NAN service discovery.
bool wifiPayload(const uint8_t *frame, size_t len, const uint8_t *&data, size_t &size);
const char *qualityText(const Info &info, uint32_t now);
void reset(Info &out);

} // namespace RemoteId
