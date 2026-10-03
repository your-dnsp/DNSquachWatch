# Flashing and microSD content — firmware v1.5.2 / content v1.5

Download and extract the firmware kit for your display. The four binaries are together at the top of the extracted folder. Firmware is flashed to the ESP32; esptool does not copy anything onto microSD.

If DNSP Content/v1.5 is already on your card, keep it: v1.5.2 changes no content files. For a new installation, download and extract the existing v1.5 microSD-content ZIP. Copy its **DNSP Content** folder to the card root. The supported path is **/DNSP Content/v1.5/**; Use the v1.5 content so the walkthrough describes this release. Eject the card safely and insert it with the CYD powered off. Missing content does not prevent boot, scanning or essential recovery, but the full walkthrough and backup installation document need it.

## Manual flashing

From the extracted firmware folder on macOS, use your actual serial port:

```sh
python3 -m esptool --chip esp32 --port /dev/cu.usbserial-100 write_flash \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

On Debian/Fedora Linux, replace the port with the one shown by the device, commonly /dev/ttyUSB0. Ensure your account has serial-device access. Install Python 3 and esptool in your preferred virtual environment. Newer esptool versions may spell subcommands with hyphens; the installation kit uses the compatible v4-style commands shown above. The flash bus uses 40 MHz; the display starts at 80 MHz and can be switched to 40 MHz in System.

## Optional installer

The script uses absolute binary paths and checks their SHA-256 before any flashing or erase. To flash only:

```sh
python3 install_dnsp.py --port /dev/cu.usbserial-100
```

To also copy the separate extracted card-content folder to a mounted card:

```sh
python3 install_dnsp.py --port /dev/cu.usbserial-100 --card /Volumes/DNSP --content /path/to/microSD-content
```

Use **--dry-run** to inspect the operation without changing anything. Content replacement is limited to this versioned content folder; unrelated files are preserved. **--replace-content** permits replacing differing bundled files after review. Never remove power or the card while the board is writing; use Safe Shutdown.

## Full erase / duress recovery

Ordinary flashing preserves PIN, duress state, Wi-Fi and stored settings. Persistent Pixel Tide remains across an ordinary reflash by design. For deliberate recovery, add **--erase**. The installer warns what will be lost and asks **[y/N]**; only y authorizes erase and flash. Declining cancels flashing. Hash validation happens first.

Full erase removes all onboard flash contents, including PIN and duress settings/state, Wi-Fi credentials, settings and onboard history. The microSD is not erased. Backups exclude authentication secrets and duress state, so having a backup does not preserve all of these. Do not use full erase for every update.

## Updates

The device uses DNSP's public repository and checks the firmware's DNSP signature and matching board target. Card content is installed manually, never fetched by the device. For future signed releases, keep the private signing key offline and use tools/sign_firmware.py; publish only images, signatures and public manifests under ota/. Do not commit private keys. Older DNSP boards using the upstream public key need this USB installation to trust DNSP-signed updates.

The v1.5 card-content download also contains the extended font and translated text. English recovery text remains built in; missing or corrupt translation records fall back to English. Copy the version-matched content before using other languages.
