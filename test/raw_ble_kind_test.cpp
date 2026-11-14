#include "raw_ble_kind.h"
#include "test_util.h"
#include <cstring>

int main() {
    suite("Raw BLE labels distinguish advertising fields, including truncated frames");
    const uint8_t nearby[] = {6, 0xFF, 0x4C, 0, 0x10, 1, 0xAA};
    const uint8_t handoff[] = {6, 0xFF, 0x4C, 0, 0x0C, 1, 0xAA};
    const uint8_t pairing[] = {6, 0xFF, 0x4C, 0, 0x07, 1, 0xAA};
    const uint8_t samsung[] = {4, 0xFF, 0x75, 0, 1};
    const uint8_t android[] = {5, 0x03, 0x0F, 0x18, 0xF3, 0xFE};
    const uint8_t servicePayload[] = {5, 0x16, 0x0F, 0x18, 0xF3, 0xFE};
    const uint8_t name[] = {3, 0x09, 0xF3, 0xFE};
    const uint8_t truncatedApple[] = {5, 0xFF, 0x4C, 0, 0x10, 3};
    ck("Nearby Info is a tentative iPhone label", rawBleKindOf(nearby, sizeof nearby) == RAW_KIND_IPHONE);
    ck("Handoff uses the same hint", rawBleKindOf(handoff, sizeof handoff) == RAW_KIND_IPHONE);
    ck("Apple pairing suggests AirPods", rawBleKindOf(pairing, sizeof pairing) == RAW_KIND_AIRPODS);
    ck("Samsung company ID", rawBleKindOf(samsung, sizeof samsung) == RAW_KIND_SAMSUNG);
    ck("Quick Share service UUID", rawBleKindOf(android, sizeof android) == RAW_KIND_ANDROID);
    ck("The same bytes in unrelated service data are ignored",
       rawBleKindOf(servicePayload, sizeof servicePayload) == RAW_KIND_NONE);
    ck("The same bytes in a name are ignored", rawBleKindOf(name, sizeof name) == RAW_KIND_NONE);
    ck("Truncated advertising element is ignored", rawBleKindOf(nearby, sizeof nearby - 1) == RAW_KIND_NONE);
    ck("Truncated Continuity element uses generic Apple",
       rawBleKindOf(truncatedApple, sizeof truncatedApple) == RAW_KIND_APPLE);
    ck("Null payload is safe", rawBleKindOf(nullptr, 1) == RAW_KIND_NONE);
    ck("Unknown devices keep the placeholder", std::strcmp(rawBleKindLabel(RAW_KIND_NONE), "(unnamed)") == 0);
    return report();
}
