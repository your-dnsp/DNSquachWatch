# DNSP v0.7 backup and recovery

This is a recovery procedure to test, not a claim that a physical recovery has already succeeded. Keep the matching DNSP release ZIP on a computer before installing any upstream update. Upstream firmware does not contain this DNSP backup UI.

## Make a backup

1. Boot DNSP and allow at least 30 seconds of responsive operation. This proves only that the main loop ran; it is not a radio/SD/endurance certification.
2. Insert the card before boot. Open Settings → Help & Recovery → Backup & Recovery → Back up to microSD. Keep power connected. Capture continues, so busy environments may increase dropped observations during the copy.
3. Firmware is copied in 1 KiB steps, then read back for a SHA-256 comparison. The whole running application slot is copied, including padding. Ten numbered directories are available, `/dnsp-backup-0` through `-9`. Existing directories are never overwritten, including incomplete attempts. A cancelled/timed-out/failed copy remains incomplete and consumes its slot until you remove it on a computer.
4. Only a verified copy receives `COMPLETE.txt`. Copy the **whole directory** to a computer and keep the matching release kit. The backup includes build name, length, app hash and the hash of the current 4 KiB partition-table sector. Hashes detect corruption, not malicious replacement.
5. Use Shutdown before removing the card or unplugging. Keep a separate copy away from the device; the same card is not disaster recovery.

`preferences.txt` contains 32 public display/alert/power/light preferences plus six language/accessibility values. It excludes PINs, WiFi credentials, mesh secrets/consent, remote-update permissions, history, device mutes, sensor IDs, favorites, touch calibration and progression. This is a public-preferences backup, **not a complete device clone**. Source code remains in the separate release source ZIP.

To restore those preferences while DNSP still runs: select the numbered slot, choose Restore Preferences, then Confirm Restore. The entire input is bounded, versioned and checksum/range checked before writing. Reboot afterwards to apply panel/rotation settings consistently. NVS values are multiple writes: interrupted restoration can be partial, so retain the file and reapply after a power interruption. Credentials are not restored. Never restore someone else's untrusted backup.

## Recover with a computer and USB

The board here is the classic 4 MiB ESP32 CYD ST7789, build `cyd-fast`. Its display clock is 80 MHz; its **flash** clock remains 40 MHz. Do not substitute an ILI9341 or other target's kit. A backup from the second OTA slot still contains an application that may be installed into app0; its old source address does not dictate the new boot selection.

Use esptool 4.5.1 (the version used in this project) and a data-capable USB cable. Determine the actual serial port; `PORT` below is a placeholder. If auto-reset cannot enter the ROM loader, use the board's documented BOOT/reset procedure. Do not use `--force` to override secure-boot/encryption or image safety checks.

Read the current partition sector before writing anything:

```sh
python3 -m esptool --chip esp32 --port PORT read_flash 0x8000 0x1000 device-partitions.bin
python3 tools/verify_backup.py --backup /path/to/dnsp-backup-0 --kit /path/to/DNSquachWatch-v0.7-cyd-fast --device-layout device-partitions.bin
python3 -m esptool --chip esp32 image_info /path/to/dnsp-backup-0/firmware.bin
```

The checker reads local files only. It checks backup hashes, kit hashes, build name, lengths, allowed offsets and the actual device layout. If a layout differs, stop for a board-specific recovery review rather than guessing offsets. The checker prints a **manual** command only after the checks pass; it does not execute it. Inspect esptool's image checksum/hash result as well.

For this verified layout, the command uses the matching kit's bootloader at `0x1000`, partition table at `0x8000`, OTA initialization image at `0xE000`, and the verified backup application at `0x10000`. The OTA initialization selects app0. It does not erase NVS, app1, the raw BlackBox region at `0x3D0000–0x3EFFFF`, or core-dump region. Firmware restoration therefore does **not** remove personal data. Do not run a blanket erase for routine recovery.

After a deliberate recovery: confirm boot/version, touch calibration, panel colors/orientation, both radios, SD writes and Settings. Restore public preferences if desired, reboot and re-check. Keep the old kit until this drill succeeds on the real device. A stock release-kit installation is also a recovery route if an SD backup fails verification.

Espressif documents application-slot/OTA-data selection in [ESP-IDF 4.4 OTA](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-reference/system/ota.html) and USB read/write/image checks in [esptool v4 basic commands](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html). The procedure also follows this project's pinned SDK and generated partition table.
