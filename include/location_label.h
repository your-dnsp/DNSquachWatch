#pragma once
#include <stdint.h>
#include <stddef.h>
namespace LocationLabel {
constexpr uint8_t MAX_TEXT=24, MAX_LABELS=64, MAX_NETWORKS=6;
struct Entry { uint32_t key=0; char text[25]{}; };
struct Network { char ssid[33]{}; uint32_t key=0; };
struct Snapshot { uint32_t magic=0x31434f4cu; Entry labels[MAX_LABELS]{}; Network networks[MAX_NETWORKS]{}; uint32_t checksum=0; };
void begin();
uint32_t currentKey();
const char* text(uint32_t key);
const char* current();
bool recalled();
const char* status();
bool set(const char* label);
bool clear();
void clearSession(); // retains saved network bindings and historical labels
const char* associatedNetwork(); // network bound to the active label, if any
bool rememberNetwork(const char* verifiedSsid); // explicit user action after authentication this boot
bool forgetNetwork(const char* verifiedSsid);
void wifi(const char* authenticatedSsid); // nullptr/empty means disconnected; never a scan result
void capture(Snapshot& out);
bool validate(const Snapshot& in);
bool restore(const Snapshot& in); // merges dictionary so retained history is never relabeled
void wipe();
}
