# DNSquachWatch v0.7 — test report

Local development draft for ESP32 CYD ST7789, experimental 80 MHz display. No GitHub push or device flashing was performed. No CYD serial port was available. Physical testing remains outstanding.

## Measured resources

| Measure | Result |
|---|---:|
| Actual cyd-fast application image | 1,845,376 bytes |
| Increase over v0.6 | 19,744 bytes (19.3 KiB) |
| Application slot size | 1,966,080 bytes |
| Remaining application image space | 120,704 bytes (117.9 KiB) |
| Static RAM | 102,864 bytes |
| Static RAM increase over v0.6 | 816 bytes |

Both OTA slots and the raw BlackBox history reservation are preserved. Generated partition-table bytes match v0.6. Display speed remains 80 MHz; flash speed is 40 MHz. Static RAM is not a measurement of worst-case live heap or stack use.

## Passed checks

- 33 native unit suites, including existing detection, security, research, language, menus, power and game checks.
- Four focused suites under AddressSanitizer and UndefinedBehaviorSanitizer: public preferences/Care, verified copy, research and security.
- Three full UI runs: landscape, portrait and reboot path.
- Two full UI runs under AddressSanitizer and UndefinedBehaviorSanitizer, one per orientation.
- A 12,000-loop accelerated mixed-screen simulator run. Virtual time advances do not represent hours of real hardware endurance.
- Six flash-layout tests and eight computer recovery-checker tests.
- Four firmware builds: cyd, cyd-ili9341, cyd-fast and awok. Only cyd-fast is packaged for this user's ST7789 target. The frozen 3.5-inch target was not tested.
- ESP image checksum and validation hash passed. Release ZIP integrity and packaged binary checksums verified.

New coverage includes preferences round-trip, truncation/corruption/extra fields, invalid values, excluded credentials, persisted favorites, next-boot greeting behavior, health counters, known SHA-256 vector, bounded copy and source/write/read-back/corruption/size failures. Research checks cover bounded readable reports, confidence and loss accounting. Recovery checks reject incomplete, corrupt, duplicate-field, wrong-board and mismatched-layout fixtures.

UI checks cover Care navigation, four screen rotations, favorites editing/launch, simulated detection isolation, lock, missing-card errors, and existing alert/game interruptions and shutdown. Screenshots were inspected in both orientations. The simulator cannot emulate the physical microSD driver, RF coexistence, electrical timing or touch-panel characteristics.

## Remaining release gates

Complete ENDURANCE-CHECKLIST.md on the actual device: extended scanning with the 80 MHz screen, minimum heap/largest block under load, gameplay while scanning, full/removed/failing SD cards, interrupted writes/restores, safe shutdown and reboot. Then perform the USB recovery drill in RECOVERY.md with a real verified backup.

The firmware backup copies the running application slot, not all flash. Preferences exclude credentials, history, calibration and progression. Public preference restore validates before writing but is not atomic across all NVS keys. Hashes establish integrity, not publisher identity. Experimental radio signatures need captured positive and negative examples; no new model is certified by these software tests.

See RELEASE-REVIEW.md for implemented changes and remaining red flags, and FEATURE-STATUS.md for deferred or unimplemented features. Existing features were retained; approval of older roadmap ideas is not a claim that they have shipped.
