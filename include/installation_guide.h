#pragma once
#include <stddef.h>
#include <stdint.h>

namespace InstallationGuide {

// Receives one decoded block. Returning false stops decoding immediately.
typedef bool (*WriteFn)(void* context, const uint8_t* data, size_t size);

// The guide stays self-contained in firmware, but is stored in a small LZ
// stream rather than as 9.9 KB of plain text. Decode uses 1 KiB of temporary
// stack only while a backup is writing the guide to microSD.
bool write(WriteFn writer, void* context);
size_t size();

}  // namespace InstallationGuide
