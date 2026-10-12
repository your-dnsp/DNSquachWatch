# Receiver hardware

**ILI9341 receiver support has been removed.** DNSP has never gifted an ILI9341 version and now uses those boards for simulation testing. DNSP plans to gift only classic 2.8-inch ST7789 CYDs, specifically selecting boards with both microUSB and USB-C ports. The simulation transmitter is a separate project.

The current receiver target is **ST7789-80MHz**, built as `cyd-fast`. A matching display controller matters; do not flash this image onto an ILI9341 board. The two USB connector types describe DNSP's preferred gift-board selection, not a way to identify every board's display controller.

ILI9341 receiver kits, board setup and update endpoints have been removed from the current repository. Historical release notes describe earlier builds and do not imply current support. The original [SquachWatch project](https://github.com/skizzophrenic/SquachWatch-CYD) supports more hardware than this edition.

See [current firmware](../../firmware/README.md).
