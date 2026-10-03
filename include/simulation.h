#pragma once
#include "state.h"
#include <stddef.h>
namespace Simulation {
// DNSP test-address convention, not a reservation or authenticated identity.
// Always inspect decoded/display-order address bytes, never packet substrings.
bool marked(const uint8_t* mac);
inline bool marked(const Detection& d) { return marked(d.mac); }
const char* roleName(AddressRole);
const char* note();
// Finish an open JSON object after its last ordinary field; main-task only.
bool finishJson(char* out,size_t cap,size_t at,const uint8_t* mac,AddressRole role);
// Derived subtag: user-owned label storage is never edited. Appends once.
void subtags(char* out,size_t cap,const char* user,const uint8_t* mac);
}
