#include "installation_guide.h"

namespace InstallationGuide {
namespace {
#include "installation_guide_data.inc"
}

size_t size() { return UNPACKED_SIZE; }

bool write(WriteFn writer, void* context) {
    if (!writer) return false;
    uint8_t history[1024]{};
    uint8_t block[128];
    size_t source = 0, produced = 0, pending = 0;
    auto emit = [&](uint8_t value) -> bool {
        history[produced & 1023u] = value;
        block[pending++] = value;
        ++produced;
        if (pending == sizeof block) {
            if (!writer(context, block, pending)) return false;
            pending = 0;
        }
        return true;
    };

    while (source < PACKED_SIZE && produced < UNPACKED_SIZE) {
        const uint8_t flags = PACKED[source++];
        for (uint8_t bit = 0; bit < 8 && produced < UNPACKED_SIZE; ++bit) {
            if (!(flags & (1u << bit))) {
                if (source >= PACKED_SIZE || !emit(PACKED[source++])) return false;
                continue;
            }
            if (source + 1 >= PACKED_SIZE) return false;
            const uint16_t packed = uint16_t(PACKED[source]) | (uint16_t(PACKED[source + 1]) << 8);
            source += 2;
            const size_t distance = (packed >> 6) + 1u;
            const size_t length = (packed & 0x3fu) + 3u;
            if (distance > produced || produced + length > UNPACKED_SIZE) return false;
            for (size_t i = 0; i < length; ++i)
                if (!emit(history[(produced - distance) & 1023u])) return false;
        }
    }
    return produced == UNPACKED_SIZE && source == PACKED_SIZE &&
           (!pending || writer(context, block, pending));
}

}  // namespace InstallationGuide
