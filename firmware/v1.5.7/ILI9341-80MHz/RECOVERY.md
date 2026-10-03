# DNSquachWatch v1.1.2 recovery

This release ships two targets: **ST7789 CYD at an initial 80 MHz display clock** and **ILI9341 CYD at an initial 80 MHz display clock**. Keep the entire matching release ZIP on a computer. Each contains the four files required to recover the board over USB. If the panel stays white, use the other 2.8-inch CYD display kit; do not mix files from the two kits.

## On-device Safe Mode

Put a finger anywhere on the display before applying power and keep holding until Recovery Safe Mode appears. It starts before normal radios and microSD logging. More than four short/failed boots inside 90 seconds also enters Safe Mode automatically; a deliberate Safe Shutdown does not count.

The microSD tools are also available during normal use under **Settings → Storage & Recovery → microSD Recovery**:

1. **Find & Remount** retries card detection without rebooting.
2. **Test Card** performs a bounded write/read/remove check.
3. **Format microSD** requires the deliberate `3`, `2`, `1`, `FORMAT NOW` sequence and erases the card. Copy recoverable files to a computer first.

## Firmware backup

Settings → Storage & Recovery → Backup & Restore copies the running application and recoverable user state to `/dnsp-backup-N`. Progress remains visible while readable logs are prepared, firmware is copied, and the copy is verified. A successful backup contains:

- `firmware.bin`
- `preferences.txt`
- `current-log.csv` — a readable snapshot of the active in-memory LOG when the backup began
- `operational-state.bin` — detection options, Ignore entries, user labels, Rules state, scan profile, and active Watch/Hunt targets
- readable stored scan, rule, and device/system history copied from `/DNSP Readable Logs/Current`
- `DNSQUACHWATCH INSTALLATION.txt`
- `COMPLETE.txt`

Only a verified backup gets `COMPLETE.txt`. Copy the whole directory and the matching release kit to a computer. The readable files and operational state can contain MAC addresses, names, labels, Watch/Hunt targets, and other identifiers; treat the directory as private data. The current LOG CSV remains informational, while Restore User State reapplies supported settings, labels, Ignore entries, rules, profiles, and active targets. Files already on the same microSD are not duplicated. PINs, Duress state, Wi-Fi passwords, mesh/authentication secrets, and touch calibration are never included.

## USB recovery

Ordinary four-image flashing preserves NVS, BlackBox history, and the separate duress journal. It therefore does not exit Pixel Tide. For an ordinary reinstall, follow `DNSQUACHWATCH INSTALLATION.txt` from the matching release kit.

To clear an active duress state or irreparably damaged saved configuration, erase the complete 4 MiB flash, then immediately flash the four release images:

```sh
python3 -m esptool --chip esp32 --port PORT erase_flash
python3 -m esptool --chip esp32 --port PORT write_flash \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

This full erase destroys all internal settings and history. It does not erase a removable microSD card.
