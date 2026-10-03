# v1.5 resource audit — ST7789-80MHz

The complete application is **1,949,040 bytes** in the unchanged **1,966,080-byte** slot: **17,040 bytes remain**. The v1.4 image was 1,960,096 bytes. These are complete .bin sizes, including headers, padding and checksum; PlatformIO's smaller ELF figure is not the installation size.

The compiler uses link-time optimization. English text/font stays onboard; extended glyphs and 702 translated strings use version-matched card files, bounded reads and content checks. No detection, Remington, BlackBox, coredump or persistent duress feature was removed. The compiled SDK TLS-server role is rejected because the device only acts as an HTTPS client.

Backup's former 6,592-byte start frame is replaced with separately scoped, non-inlined helpers. Measured helper frames are 320 bytes for partition hashing, 752 for the current log, 1,152 for operational settings, 1,248 for public preferences and 2,336 for location metadata. The loop frame is 1,328 bytes; Backup::tick is 2,304 bytes. These are compiler stack-frame measurements, not proof of worst-case runtime stack use. Hardware stack high-water checks remain necessary.

GitHub update checks temporarily lend the display buffer to the TLS client; cancellation waits for task cleanup before radio/display recovery. The update task reserves 12 KiB. Static RAM and complete symbol data are recorded in SIZE-AUDIT-v1.5-ST7789.json; dynamic radio/TLS/SD heap peaks require hardware measurements.
