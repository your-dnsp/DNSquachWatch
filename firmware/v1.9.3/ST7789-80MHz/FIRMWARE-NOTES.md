DNSquachWatch v1.9.3 — ST7789-80MHz, SquachWatch base v1.40.0

Integrates timed per-device Snooze. Tap once for 10 minutes, twice for 30 minutes,
three times for one hour, four times for four hours; the next tap cycles to ten
minutes. The card shows the selected duration and TAP AGAIN: LONGER. Any fresh
tap on the card advances the choice, which confirms three seconds after the
last tap. A held finger does not repeat. Classification, acquisition, evidence
and logging continue. Permanent Ignore and DNSP's separate ten-minute Snooze
All remain available. Timed entries clear on restart and do not write NVS.
A full list reports failure instead of replacing another device's active timer.

Applies background-character speech-font fixes while preserving DNSP visuals,
menus, labels/tags, OUI research/submissions, location labels, saved-Wi-Fi behavior,
simulation provenance, signed DNSP updates, security and coordinated SD/touch,
backup/recovery and Safe Shutdown. Unsupported LoRa/GPS/WiGLE, other-board and
web-flasher/telemetry changes are excluded. ST7789 receiver only.

Complete image: 1,962,736 bytes; app slot: 1,966,080; 3,344 bytes remain.
Static RAM: 124,464 bytes, 152 above v1.9.2. Snooze uses a bounded
32-address table and parallel deadlines rather than padded per-entry structures.
No storage-layout, backup-schema or card-content change.

Flash normally without ESP32 erase. Keep DNSP Content/v1.5 unchanged; there is no
new microSD folder to copy. Serial baud is 115200; display starts at 80MHz and
flash remains 40MHz.

Validation includes all native test groups, changed-Snooze sanitizer tests,
exact expiry/rollover/full-capacity and Ignore-precedence checks, actual receiver
loop touch tests, held-finger behavior, tap cycling, PIN-lock behavior, rendering
in both orientations, prior UI/font/companion/band checks, driver-adapter checks,
content/version/layout/installer checks and image/package integrity.
Physical touch, RF and card timing still require verification on this build.
