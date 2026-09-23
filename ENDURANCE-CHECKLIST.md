# Hardware endurance and gift acceptance

Status: physical test NOT RUN. No CYD USB serial port was visible during this release. An accelerated desktop run is useful for state/logic/memory errors but cannot measure real radio coexistence, SD latency, power or 80 MHz display stability.

Use one expendable test card and keep real data/backups on a computer. Record board revision, panel, firmware build, power supply, card model/format/capacity, ambient conditions and test duration. Use ordinary owned test devices; do not transmit fake surveillance identities to strangers.

1. Baseline: boot with card, verify touch/colors, choose 80 MHz cyd-fast, inspect Device Health, microSD Status and Diagnostics. Export `/dnsp-health.txt`. Record free heap and largest block after initialization.
2. Two hours scanning: compare BLE/WiFi reception to the board's own baseline in the same room. Reopen settings/help, rotate, dim/wake and lock/unlock. Watch queue/pressure drops. A stable empty room cannot prove every model is detected.
3. Thirty minutes playing Squach Snacks: run scanning in the background; verify genuine test-device alerts pause the round, the steering touch cannot dismiss alerts, and returning preserves score/lives/ball. Record drops and memory again.
4. Storage: complete several research sessions, inspect CSV/JSONL plus readable field reports and counters. Test card absent at boot, full test card, and removal during a test write. Verify clear failure feedback and recovery after reboot. Sudden removal can corrupt FAT; use the expendable card only.
5. Backup faults: complete a backup and verify it on a computer. Cancel another copy; interrupt power on an expendable attempt. The old completed directory must remain unchanged and the new attempt must lack a valid completion marker. Try invalid/truncated preferences; no values should be applied. Test reboot after a legitimate restore.
6. Safe power: shutdown while records are queued; wait for confirmed completion before unplugging. If completion is unconfirmed, do not describe the shutdown as successful. Reboot and verify readable files.
7. Recovery drill: follow RECOVERY.md on this same board only after authorizing a flash. Verify the recovered version and all essential functions. Keep the source/release kit off-device.
8. Overnight, ideally 8–12 hours: scan/desk idle with periodic interactions and repeated sessions. Export health before/after, recording any resets, watchdogs, screen artifacts, card errors, sustained heap decline and queue growth. Compare the 80 MHz build with the existing normal-speed ST7789 build if artifacts occur.

Acceptance: no crashes or spontaneous resets; readable storage files; no false “verified” backup; correct alert/lock behavior; no progressive memory loss; no unexplained sustained detection loss versus baseline; usable touch and readable text in both orientations. No universal numeric minimum heap or drop threshold is claimed without a measured baseline. Record failures, reproduce them, and fix them before giving units away.

Gift preparation is a checklist and introduction, not a secure erase. Inspect/review saved networks, PIN and squad credentials, history stores, telemetry configuration and card files. A fresh card plus a deliberate reviewed reset is preferable to assuming “clear visible log” sanitizes everything. PIN locking does not encrypt storage. Arm the hello only after preparation; run practice cards with the recipient.
