# DNSP firmware downloads

**Current: v1.8.1 (ST7789 upstream integration)**, based on SquachWatch v1.35.0.

- [ST7789 80MHz complete flashing kit](v1.8.1/ST7789-80MHz/DNSquachWatch-v1.8.1-ST7789-80MHz.zip)
- [ILI9341 80MHz complete flashing kit](v1.7.4/ILI9341-80MHz/DNSquachWatch-v1.7.4-ILI9341-80MHz.zip)

v1.8.1 integrates upstream v1.31–v1.35 for ST7789 while retaining DNSP features and its signed GitHub updater. Hardware validation is pending. ILI9341 is not rebuilt in this update. Both receiver variants use the DD:DD:DD simulation marker.

Both kits contain all four flashing binaries, checksums, the optional installer and recovery instructions. If your classic CYD shows a solid white screen with one driver, try the other kit. Display starts at 80MHz; System can switch it to 40MHz. Flash bus remains 40MHz.

Keep existing **DNSP Content/v1.5**. New installations use [the existing content ZIP](v1.5/DNSquachWatch-v1.5-microSD-content.zip). Firmware flashing does not install card content. Card packages are only regenerated when their content changes.

v1.7.0 merges the classic-CYD features of upstream v1.29 and v1.30 while preserving DNSP's detection, recovery, menus and card-content contract. Both builds need hardware confirmation.

See [GitHub Releases](https://github.com/your-dnsp/DNSquachWatch/releases), [installation](../docs/user/INSTALLATION.md) and [changelog](../CHANGELOG.md).
