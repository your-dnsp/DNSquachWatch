#pragma once
#include <stddef.h>
#include <stdint.h>

namespace InstallationGuide {

// Receives one decoded block. Returning false stops decoding immediately.
typedef bool (*WriteFn)(void* context, const uint8_t* data, size_t size);

// Verified version-specific card content; missing/corrupt content fails the backup.
bool write(WriteFn writer, void* context);
size_t size();

}  // namespace InstallationGuide
