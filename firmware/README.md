# DNSP firmware downloads

**Current: v1.5.7**, based on SquachWatch v1.28.0.

- [ST7789 80MHz complete flashing kit](v1.5.7/ST7789-80MHz/DNSquachWatch-v1.5.7-ST7789-80MHz.zip)
- [ILI9341 80MHz complete flashing kit](v1.5.7/ILI9341-80MHz/DNSquachWatch-v1.5.7-ILI9341-80MHz.zip)

Both kits contain all four flashing binaries, checksums, the optional installer and recovery instructions. If your classic CYD shows a solid white screen with one driver, try the other kit. Display starts at 80MHz; System can switch it to 40MHz. Flash bus remains 40MHz.

Keep existing **DNSP Content/v1.5**. New installations use [the existing content ZIP](v1.5/DNSquachWatch-v1.5-microSD-content.zip). Firmware flashing does not install card content. Card packages are only regenerated when their content changes.

Host tests and both builds passed. v1.5.7 microSD responsiveness needs hardware confirmation. ST7789 backup completion was confirmed on v1.5.1; ILI9341 hardware validation remains pending. Earlier kits are retained in version folders. v1.5 is superseded because backup could crash.

See [GitHub Releases](https://github.com/your-dnsp/DNSquachWatch/releases), [installation](../docs/user/INSTALLATION.md) and [changelog](../CHANGELOG.md).
