# DNSquachWatch v1.5.2 — ST7789, 80MHz

Local build based on SquachWatch v1.28.0. This package includes all four flashing binaries. Existing DNSP menus, detection intelligence, Remington, security and the hardware-tested v1.5.1 backup fixes remain included.

## What changed

- Connecting saved Wi-Fi opens a connection-only screen and returns to saved networks. Password entry preserves this mode. It never requests updates.
- Startup connects saved Wi-Fi for time and label recall, including PIN-locked startup. Updates require an explicit firmware update action; automatic Update Check was removed from System.
- Location storage writes only occupied label/network entries, rather than a 2296-byte padded dictionary. Existing labels load and migrate after a successful save. Failed saves retain the previous label/mapping and show a warning; successful saves show LOCATION SET.
- Upstream v1.28.0 adds Over 9000, leaner character/aquarium rendering and support for sharing the seventeenth outfit. This release remains for the classic ESP32 CYD; the upstream C5 target is not added.
- Splash, System Info, serial banner and new backup manifests identify v1.5.2 / base v1.28.0.

## Flash normally, keeping existing data

Keep the existing `/DNSP Content/v1.5/` on your microSD. Card content did not change. No separate content folder is included. Do not erase the ESP32 or format the card for this update.

Exit your serial monitor. Extract this ZIP and open a terminal in this folder. Replace the serial port with your device's port:

```sh
python3 -m esptool --chip esp32 --port /dev/cu.usbserial-210 write_flash \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

Normal flashing preserves settings, PIN/duress state and onboard history. Display clock is 80MHz; flash clock is 40MHz. The included installer can verify the binaries: `python3 install_dnsp.py --port /dev/cu.usbserial-210`.

## Hardware checks

1. Connect Home Wi-Fi using Wi-Fi Networks. Confirm the page is titled WI-FI CONNECTION and has no update prompt. Return to the saved-networks list.
2. Open Alerts & Detections > Set Location. Select Home and confirm LOCATION SET. Try a custom label as well.
3. Restart with the saved home network available. Confirm the assigned label is recalled; check time after synchronization.
4. Clear the label, restart and confirm it does not return.
5. Retest backup with your usual microSD.

A network name can be reused elsewhere; a recalled label is a remembered association, not GPS. Without a saved association or a manually set label, logs use no-label-set. Manual session-only labels without a network association are not carried across reboot.

Compilation, host regressions and landscape/portrait UI simulations passed. Complete image: 1,956,848 bytes of 1,966,080 (9,232 bytes free). Physical Wi-Fi/label validation is still required. The recording established a save failure but did not identify the exact on-board NVS error. If labels still fail, retain serial output at 115200 baud; do not erase settings as a first troubleshooting step.

Original SquachWatch: Talking Sasquach / skizzophrenic. DNSP modifications: dnsprincess. Research sources: ReconGrunt, zmattmanz, Ringmast4r (especially OUI intelligence), and rpriven. See the project README and LICENSE.
