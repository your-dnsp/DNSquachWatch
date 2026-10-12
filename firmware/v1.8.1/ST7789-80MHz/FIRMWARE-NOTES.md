# DNSquachWatch v1.8.1 — ST7789-80MHz

Based on SquachWatch v1.35.0; integrates the applicable classic-CYD v1.34/v1.35 changes on top of DNSP v1.8.0. Smoother pet movement, character placement corrections, dedicated BRIGHT MAX buttons and Pocket Mode are included. A manually darkened screen waits for BOOT; later alerts temporarily borrow it for the configured DNSP duration, and a lit card still accepts dismissal taps. Long-press Charge Mode keeps its existing storage/PIN protections. Startup-held BOOT is ignored until release.

## DNSP conflict decisions

Preserve connection-only saved Wi-Fi, network labels, signed GitHub OTA, startup Safe Mode, calibration recovery, banded drawing, runtime display speed, PIN/duress, backups, Safe Shutdown, per-transmitter DEAUTH, Ignore/Snooze, exact DD:DD:DD simulation evidence, full history names, research exports and built-in artwork. New pet hops/ball rolling honor Reduced Motion. Settings organization is unchanged.

No WiGLE uploads, GPS-backed wardriving exports, phone GPS relay or coordinate-over-name history storage is introduced. Manual labels are not coordinates. Speaker, battery lamp, S3 power-off and additional-board display support are outside the supported classic 2.8-inch CYDs. This package does not add speaker settings or claim support for Freenove S3 boards.

## Installation and content

Keep DNSP Content/v1.5. No card-content or partition change. Use the usual four-file flashing command without an ESP32 erase. Saved settings are retained. Display defaults to 80 MHz, selectable down to 40 MHz; flash remains 40 MHz. Serial is 115200 baud.

## Validation and resources

ST7789 compilation and complete-image slot verification passed. All 55 native tests passed, including Pocket Mode duration, existing-versus-later alerts, disabled wake and millis rollover. All 12 Python validation scripts passed. Real setup/loop UI tests, new brightness hit regions in both orientations, upstream pet/accessibility tests, banded-render comparisons and recovery UI checks passed. built-in artwork pixel verification and all card-content hashes passed. Internal release stamps use 1.8.1 and upstream identity 1.35.0.

Complete application image: 1,948,816 bytes. Slot: 1,966,080 bytes. Spare: 17,264 bytes. Static RAM reported by the build: 124,312 bytes, 256 fewer than v1.8.0. Row tracking is bounded to the supported 320-pixel maximum portrait height, saving 800 bytes versus larger-panel reservation. Detection/history capacities are unchanged. The image is 13,744 bytes larger than v1.8.0 and 11,968 bytes smaller than v1.7.7.

## Hardware checks

Hardware testing is pending for BOOT/Pocket Mode, temporary alert wake/dismissal, brightness buttons, pet motion, both orientations, backup and Safe Shutdown. ILI9341 is not rebuilt for this update.

