# DNSP firmware downloads

**Current: v1.6.5**, based on SquachWatch v1.28.0.

- [ST7789 80MHz complete flashing kit](v1.6.5/ST7789-80MHz/DNSquachWatch-v1.6.5-ST7789-80MHz.zip)
- [ILI9341 80MHz complete flashing kit](v1.6.5/ILI9341-80MHz/DNSquachWatch-v1.6.5-ILI9341-80MHz.zip)

Both kits contain all four flashing binaries, checksums, the optional installer and recovery instructions. If your classic CYD shows a solid white screen with one driver, try the other kit. Display starts at 80MHz; System can switch it to 40MHz. Flash bus remains 40MHz.

Keep existing **DNSP Content/v1.5**. New installations use [the existing content ZIP](v1.5/DNSquachWatch-v1.5-microSD-content.zip). Firmware flashing does not install card content. Card packages are only regenerated when their content changes.

v1.6.5 repairs recovery navigation, result visibility and card-test reporting on both supported displays. It retains the established card driver and 4 MHz clock; temporary diagnostics and unsuccessful reset experiments are excluded. ST7789 hardware confirmation covers the retained recovery implementation and backup completion on a working card. ILI9341 hardware confirmation remains pending.

See [GitHub Releases](https://github.com/your-dnsp/DNSquachWatch/releases), [installation](../docs/user/INSTALLATION.md) and [changelog](../CHANGELOG.md).
