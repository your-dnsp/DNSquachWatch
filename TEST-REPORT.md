# DNSquachWatch v1.1.2 test report

Targets: 2.8-inch CYD, ST7789 and ILI9341, initial 80 MHz display clock (`cyd-fast` and `cyd-ili9341-fast`). Upstream base: SquachWatch v1.25.0.

## Automated results

- PlatformIO `cyd-fast` and `cyd-ili9341-fast` release builds: passed.
- ST7789 firmware flash use: 1,912,213 / 1,966,080 bytes (97.3%); 53,867 bytes remain by the linker measurement. Its generated `firmware.bin` is 1,918,512 bytes, leaving 47,568 bytes of raw slot headroom.
- ILI9341 firmware flash use: 1,912,313 / 1,966,080 bytes (97.3%); 53,767 bytes remain by the linker measurement. Its generated `firmware.bin` is 1,918,608 bytes, leaving 47,472 bytes of raw slot headroom.
- Static RAM use for each target: 120,128 / 327,680 bytes (36.7%).
- Installation-guide decode passed a byte-for-byte comparison with the printable source; the compact aquarium texture passed the full firmware build.
- Complete host unit suite: passed, including Remote ID, malformed radio input, ALPR/signature matching, user-label validation and replacement, the 90-second Sketchy Environment rule, security, duress journal interruption, verified backup, spam flood, alert queue/snooze, settings persistence, crash reports, research exports, and game logic.
- All 35 Python partition, display-target, duress-journal, recovery-tool, Bluetooth-runtime, and display-runtime tests passed.
- Focused DEAUTH tests passed: same-source threshold, unrelated/split sources, independent per-source cooldowns, true sliding-window expiry, multiple receivers, BSSID/reason/protection evidence, malformed frames, rollover arithmetic, and deterministic table eviction. Research tests passed for decoded RAW evidence, address-free redaction, protected-body handling, and coherent-burst session counters.
- Backup now refreshes only internal readable history instead of copying microSD files back onto the same card. Restore validates both records before applying them, journals the selected slot for deterministic retry, verifies restored state, and hands Watch/Hunt targets across one reboot. Hardware power-interruption verification remains required.
- Banded-frame composition simulation passed in landscape and portrait, and the isolated duress simulation passed; the decoy path does not start normal detection services.
- The broad UI integration harness still reports pre-existing failures in alert timing/snooze, several simulator preference-persistence checks, and Hebrew persistence. The same failures reproduce on the unmodified v1.1.0 simulator, while the v1.1.2 Research controls and all DEAUTH-specific tests pass. This release does not claim those unrelated harness failures were repaired.

Compiler warnings remaining are simulator/library warnings for unavailable hardware paths and TFT_eSPI's built-in touch warning. The board uses the separate XPT2046 touch library, so TFT_eSPI's own touch API is intentionally disabled.

## Physical checks still required

The preceding images passed initial testing on the user's ST7789 board, including ordinary boot, display, microSD visibility, and the tested recovery paths reported during development. These exact v1.1.2 images have not yet been flashed, and the ILI9341 build has no physical-panel result yet. On each display, verify sustained boot with microSD inserted; touch and color calibration; a controlled DEAUTH RAW Research session and redacted export; label exports; the paired ALPR/deauth rule; scan/system history navigation; two ordinary Refresh runs without duplicates or removed history; an interrupted Refresh followed by successful retry; Export & Organize; Pocket Reader access; interrupted and successful backup/restore of settings, labels, Ignore entries, rules and Watch/Hunt targets; the one-reboot target handoff; all five scan-profile controls; the three-minute comparison; and unavailable/full/removed-card failures before wider distribution.

Avata 2 reception and human-reviewed translations remain separately deferred.
