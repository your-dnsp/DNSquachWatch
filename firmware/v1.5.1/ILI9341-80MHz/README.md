# DNSquachWatch v1.5.1 — ILI9341, 80 MHz

This corrective build addresses the loopTask stack overflow confirmed during v1.5 backup. It preserves DNSP menus, detection rules and existing card content. It uses the same backup correction that DNSP confirmed on the physical ST7789-80MHz device. The ILI9341 build passed compilation and image checks; testing on an ILI9341 device is still required.

**Keep your existing /DNSP Content/v1.5/ folder. Its content has not changed. No separate card-content download is needed for this update.**

## Flash

Exit the serial monitor, extract this ZIP, and open a terminal in this folder. All four binaries are here. Use your actual port:

```sh
python3 -m esptool --chip esp32 --port /dev/cu.usbserial-210 write_flash \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

Ordinary flashing retains onboard settings, PIN/duress state and history. Do not erase flash or format microSD for this fix. The display uses 80 MHz; the flash bus remains 40 MHz. The optional installer verifies image hashes: `python3 install_dnsp.py --port /dev/cu.usbserial-210`.

## Retest backup

Power on with the existing microSD. After 30 seconds, start Backup & Restore → Backup to microSD. Allow all history-copy and firmware-verification stages to finish. Success must show a verified backup folder such as /dnsp-backup-3; an interrupted directory without COMPLETE.txt is not a complete backup.

Earlier incomplete backup folders remain for inspection. There are ten backup slots. If all are occupied, copy them to a computer and remove only unwanted incomplete folders before retrying. Do not delete scan logs, readable-history journals or DNSP Content.

The included verify_backup.py provides additional recovery checks using the backup folder, matching release kit and a partition-sector read from the device; see RECOVERY.md for its required arguments. For a remaining failure, capture output with `python3 -m serial.tools.miniterm /dev/cu.usbserial-210 115200`; start backup and retain the panic lines. Do not use 2,000,000 baud on this connection.

## Changes and validation

History preparation and file-copy work have separate stack frames. A fixed cursor advances through the captured flash history with at most eight 64-byte reads per step, rather than rescanning all earlier records. A scheduler tick is yielded between backup steps. New empty readable-history collections avoid unnecessary legacy duplicate scans; interrupted and existing collections retain recovery/duplicate checks.

Host regression tests, full 1,800-event history export, retry/duplicate checks, snapshot overwrite handling, card write failures, installer safety checks and UI simulations passed. ESP32 compilation and complete-image validation passed. Firmware size: 1950176 bytes of 1,966,080 (15904 bytes free). This package is for ILI9341 only. Use the separate ST7789 kit for ST7789 panels. If your CYD shows a solid white screen, try the other display-driver kit; keep each kit’s four binaries together.

The original SquachWatch project is by Talking Sasquach (skizzophrenic). DNSquachWatch modifications are by dnsprincess/DNSP. Device research credits include ReconGrunt, zmattmanz, Ringmast4r (especially the OUI intelligence), and rpriven. See the project README and LICENSE for additional context.

Package correction (2026-10-02): the splash now shows DNSP v1.5.1 using the build version. This replaces the earlier same-version image; backup and card content are unchanged.
