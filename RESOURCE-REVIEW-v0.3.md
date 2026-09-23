# v0.3 resource review

CYD ST7789 `cyd-fast`: static RAM 101,936 / 327,680 bytes; application flash 1,820,509 / 1,966,080 bytes. Application headroom: 145,571 bytes. These are linker figures, not measured runtime free heap or stack peaks. v0.2 used 99,744 bytes static RAM and 1,739,617 bytes flash; v0.3 adds features and therefore remains larger overall despite optimizations.

- Static Field Tools screens redraw on change. Live field readings refresh at most four times per second. Transition redraws remain intact. A simulator regression checks idle redraw suppression and language invalidation.
- The 646-glyph font representation uses a shared 31,424-byte bitmap blob plus approximately 5,168 bytes of index entries instead of 43,928 bytes of padded records: approximately 7,336 bytes saved. This compares representations of the same new glyph catalog, not total firmware sizes across versions.
- Existing 8-bit display sprite and changed-row output remain. No second full-screen buffer was introduced. No features were removed for this cleanup.
- Observation queue: six bounded items, up to 228 payload bytes each. Four aircraft slots, four discovered sensors and four selected sensors. Telemetry packets are capped at 512 bytes and four per loop. Radio callbacks enqueue; SD writes occur in the main loop.
- New code is consistently formatted and protocol parsing is separated from hardware/network storage adapters. DEX table completeness is enforced at compilation.

## Pins and clocks

No new GPIOs are allocated. Display: MISO12, MOSI13, SCLK14, CS15, DC2, backlight21. Touch: SCK25, MOSI32, MISO39, CS33, IRQ36. microSD: SCK18, MISO19, MOSI23, CS5. These are occupied resources, not free expansion pins.

The selected experimental display clock remains 80 MHz; flash clock remains 40 MHz. Neither compile tests nor simulation establish electrical stability. Test display corruption, touch responsiveness and card writes together on the real board. No new pin assignment or hardware modification is required.

## Remaining measurements

Measure minimum free heap, largest free block, stack high-water marks, dropped observations, frame latency and SD stalls during crowded scanning and prolonged operation. Physical radio/card performance has not been benchmarked. Flash capacity is now tight; further additions need size budgeting. Broader legacy redraw refactoring should follow measurements rather than remove effects or features speculatively.
