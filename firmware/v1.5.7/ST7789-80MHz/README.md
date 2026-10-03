# DNSquachWatch v1.5.7 — ST7789, 80 MHz

This release reports the actual slow history step and its slowest individual file operation. It also avoids checking the location schema marker for every committed record, retaining the paced automatic refresh from v1.5.6. Two alternating CRC-checked checkpoint files preserve interruption recovery. Manual backup keeps its existing pace. Receiver simulation labeling and consistent firmware identification remain. Existing DNSP features and recovery journals remain compatible.

Keep `/DNSP Content/v1.5/` on your card. Its contents have not changed.

Extract this ZIP and run the following from this folder, using your actual serial port. All four flash files are included. Exit the serial monitor before flashing.

```sh
python3 -m esptool --chip esp32 --port /dev/cu.usbserial-210 write_flash \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

Normal flashing preserves settings, PIN/duress state and onboard history. Do not erase flash or format the card for this update. Serial diagnostics use 115200 baud.

Both display builds, all 53 host test groups, shared-version checks, image integrity checks and UI checks passed. Hardware responsiveness still needs confirmation. Observe the main screen with the card installed for several minutes, dismiss a simulated detection, then complete a backup and Safe Shutdown. See FIRMWARE-NOTES.md for remaining limitations.

Firmware: 1,962,096 bytes of the 1,966,080-byte app slot (3,984 bytes free).

Original SquachWatch: Talking Sasquach / skizzophrenic. DNSquachWatch modifications: dnsprincess / DNSP. Research credits include ReconGrunt, zmattmanz, Ringmast4r (especially OUI intelligence), and rpriven.
