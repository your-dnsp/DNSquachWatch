# DNSP firmware downloads

**Current: v1.8.3 (ST7789-80MHz)**, based on SquachWatch v1.35.0.

- [Current ST7789 80MHz complete flashing kit](v1.8.3/ST7789-80MHz/DNSquachWatch-v1.8.3-ST7789-80MHz.zip)
- [Hardware scope and board selection](../docs/user/HARDWARE.md)

**ILI9341 receiver support has been removed.** DNSP has never gifted an ILI9341 version and now uses those boards for simulation testing. DNSP plans to gift only classic 2.8-inch ST7789 CYDs, specifically selecting boards with both microUSB and USB-C ports. The simulation transmitter is a separate project.

v1.8.3 separates microSD and display controllers, coordinates resistive touch through software SPI, and retains upstream v1.35, DNSP features and the signed GitHub updater. Physical verification of the exact release remains pending.

The current kit contains all four flashing binaries, checksums, the optional installer and recovery instructions. Display starts at 80MHz; System can switch it to 40MHz. Flash remains at 40MHz. Confirm the display controller before flashing.

Keep existing **DNSP Content/v1.5**. New installations use [the existing content ZIP](v1.5/DNSquachWatch-v1.5-microSD-content.zip). Firmware flashing does not install card content. Card packages are only regenerated when their content changes.

See [GitHub Releases](https://github.com/your-dnsp/DNSquachWatch/releases), [installation](../docs/user/INSTALLATION.md) and [changelog](../CHANGELOG.md).
