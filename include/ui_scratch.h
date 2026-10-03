#pragma once
#include "pixel_tide.h"
namespace UiScratch {
// Pocket Reader and the persistent duress decoy never run together.
// Each entry path activates and clears its own member. No radio, storage,
// backup, or background job uses this memory; no heap allocation is needed.
struct Reader {char names[16][96];char text[1025];};
union Storage {int16_t tide[2][PixelTide::H][PixelTide::W]{};Reader reader;};
extern Storage storage;
}
