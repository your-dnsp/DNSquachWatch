# Build and validate DNSquachWatch

DNSP release targets are the classic 2.8-inch ESP32 CYD: `cyd-fast` (ST7789, initial 80 MHz) and `cyd-ili9341-fast` (ILI9341, initial 80 MHz). Other inherited environments are not validated DNSP release targets.

For prebuilt firmware, use [the release kits](../firmware/README.md) and [installation guide](user/INSTALLATION.md). Building from source is optional.

## Requirements

- Python 3 and PlatformIO Core 6.1.18 (or the PlatformIO extension).
- A C++ compiler and Make for native tests.
- Internet access for the first dependency installation; the firmware itself does not fetch microSD content over Wi-Fi.

The project pins Espressif32 6.5.0, Arduino-ESP32 2.0.14, TFT_eSPI 2.5.43 and NimBLE-Arduino 2.5.1. Keep these pinned dependencies unless deliberately porting and validating the firmware.

## Build

Run from the repository root:

```sh
python3 -m pip install platformio==6.1.18
pio run -e cyd-fast
pio run -e cyd-ili9341-fast
```

Binaries are generated under `.pio/build/<target>/`. Use the complete matching release kit for end-user flashing; never mix binaries from different kits. Both CYD environments use serial diagnostics at **115200 baud**.

`platformio.ini` selects the partition tables in `config/partitions/` and build adapters under `tools/`. The layout checker protects settings, crash storage, BlackBox history and the persistent duress journal. Image-size checks reject oversized builds. Generated size reports and local build output are not committed.

## Validate

```sh
make -C test -j2
python3 test/installer_test.py
python3 test/install_tools_test.py
python3 tools/check_language_content.py
```

Additional layout and display-configuration checks are available in `test/flash_layout_test.py` and `test/display_targets_test.py`. See [validation records](validation/) and [known limitations](validation/KNOWN-LIMITATIONS.md). Hardware testing remains necessary before treating a build as field validated.

## Content and signing

Keep supplied `DNSP Content/v1.5` files on microSD as described in the installation guide. The printable guide source is `docs/user/DNSQUACHWATCH INSTALLATION.txt`; `tools/pack_installation_guide.py` retains its historical packing utility. Signed update manifests live in `ota/`; private signing keys must never be committed. Published kits and their checksums are kept intact during repository documentation cleanup.
