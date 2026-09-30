# DNSquachWatch v1.1.2 feature status

This file describes the current v1.1.2 ST7789 and ILI9341 80 MHz images. Older planning and measurement documents are retained as historical records and do not define this release.

| Area | Current status | Remaining validation or work |
|---|---|---|
| 15/30/45/60-second alerts, default 30, queue, and Snooze All | Implemented | Physical burst testing |
| Why This Matched and qualitative confidence | Implemented | Continue model-specific positive and negative samples |
| Flock, Raven, Axon, Meta, ALPR, camera, and tracker signatures | Experimental detection support | Field validation; a match remains a clue |
| Remote ID over BLE/Wi-Fi and focused DroneWatch | Implemented | Avata 2 retest deferred; no 5.8 GHz receiver |
| ELRS/FPV equipment clues and pit tools | Implemented | Test named Air65, RadioMaster, and Fat Shark equipment |
| Research sessions, notes, raw/redacted exports, DEAUTH evidence, and field report | Implemented | Card-removal, full-card, and field-capture testing |
| Per-source Wi-Fi DEAUTH burst detection | Implemented: 6 frames / 3 seconds / 15-second per-source cooldown | Validate against controlled ordinary disconnects and controlled bursts |
| Stored Alert History and boot/crash/system view | Implemented | Physical navigation and rollover testing |
| Sketchy Environment ALPR + deauth rule | Implemented | Test both event orders and 90-second boundaries |
| Readable Log Export and Pocket Reader | Implemented with stable IDs and retry journal | Interrupt power during refresh and confirm recovery |
| Comprehensive microSD backup and restore | Implemented; security secrets excluded | Physical restore and interruption drill |
| Watch/Hunt restore | Implemented with one-reboot handoff | Confirm target survives the first restart and is session-only afterward |
| Stationary/Balanced/Fast/Maximum/Custom profiles | Implemented | Compare reception and stability on hardware |
| microSD status, remount, test, format, and error logging | Implemented | Failing-card and full-card tests; volume label unavailable |
| PIN, lock, Forgot confirmation, Wipe After 10 explanation | Implemented | Physical usability checks |
| Duress PIN and persistent Pixel Tide | Implemented and host-tested | Destructive expendable-card test; no secure-erasure claim |
| Safe Shutdown and Reboot | Implemented | Power-removal and interrupted-write checks |
| Safe Mode and repeated-failed-boot recovery | Implemented | Manual and automatic hardware entry checks |
| Menu organization and accessibility shortcuts | Implemented | Longer real-user usability pass |
| Language previews: EN/ES/FR/DE/JA/ZH and hidden Hebrew | Partial translation framework | Human translation review deferred |
| Breakout / Squach Snacks | Implemented | Long radio-plus-game performance test |
| DNSP Screen Light, Morse, randomizers, timer/counter, Radio Activity | Implemented | Physical control/layout check |
| 80 MHz ST7789 and ILI9341 displays | Implemented as selected experimental defaults; runtime 40 MHz fallback | ST7789 regression and ILI9341 physical-panel/long display-touch-card soak tests |
| Wi-Fi firmware updating and USB recovery | Implemented | Matching-kit recovery drill |
| Bluetooth firmware updating on this CYD | Removed | Wi-Fi and USB remain available |
| WiGLE | Parser and policy groundwork only | External GPS hardware is deferred |
| OSRS font | Planned | Licensed glyph/resource work |
| Additional RuneScape dialogue, quests, pets, and unlocks | Backlog | Not implied by earlier planning notes |
| Beacon spam and Doom | Not implemented | None planned for this release |

The firmware remains self-contained for boot, scanning, interface, recovery, and help. microSD content is optional and is not required to flash or start the device.
