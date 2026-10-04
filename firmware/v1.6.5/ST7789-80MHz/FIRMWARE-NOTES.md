MicroSD recovery fixes for ST7789-80MHz and ILI9341-80MHz.

- Recovery help text now pages correctly; Back remains available while storage tools are busy.
- A working screen appears before card operations, followed by a visible result, including temporary-file creation failures.
- Mounting validates filesystem and root-directory access; remounting clears stale mount state.
- Accessibility now includes the shared Glitch Effects setting.

Existing `DNSP Content/v1.5/` remains compatible. No new card-content package or ESP32 erase is required. Temporary diagnostics and unsuccessful reset-command experiments are excluded.

Both display builds, native regression tests, focused display checks in both orientations, version checks and image/signature validation passed. Recovery and backup were observed working on ST7789 with compatible cards. The exact release images and ILI9341 still need hardware verification. This does not claim compatibility with the green card batch that failed initialization.

Application images: ST7789 1,963,344 bytes; ILI9341 1,963,392 bytes. Each fits the existing 1,966,080-byte app slot. Partitions and microSD content are unchanged.
