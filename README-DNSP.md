# DNSquachWatch v0.7-draft

Local foundation draft for DNSP, based on SquachWatch **1.19.1**, reviewed commit `23ff05b111ecec2eb275287946b259837ad490d5`. This source is prepared for the private DNSquachWatch repository; no device has been flashed.

**Your target: `cyd-fast`, CYD ST7789, experimental 80 MHz display clock.** This retains the original pinout and partition table. The ESP32 flash chip still uses its normal 40 MHz setting; that is separate from the display clock.

## v0.7 Help, recovery and Squach Snacks

Settings → **Help & Recovery** contains backup/restore, practice detections, Why No Match, gift preparation, field reports and device health. **Four Favorites** is at the top of Settings; tap Edit to choose four destinations. Breakout now has a forest-snack theme.

Read [RELEASE-REVIEW.md](RELEASE-REVIEW.md), [FEATURE-STATUS.md](FEATURE-STATUS.md), [RECOVERY.md](RECOVERY.md), [ENDURANCE-CHECKLIST.md](ENDURANCE-CHECKLIST.md) and [TEST-REPORT.md](TEST-REPORT.md). This remains a local draft: physical endurance, SD fault tests and a recovery drill are outstanding. Firmware backups are application-only; the settings backup is deliberately public-preferences-only. Gift preparation is a checklist, not an automated erase.

## v0.6 Menus

Alerts & Detection and Games & Squachy now have dedicated pages. Language, Accessibility, microSD Status and Shutdown / Reboot are directly reachable from Settings. Each page remembers its own folded sections and scroll position during a visit. See MENU-REVIEW.md for the full navigation map.

## v0.5 Breakout

Open **Settings → Games & Squachy → Breakout**. Tap the court to launch, then drag horizontally anywhere on the court to steer. Clear 32 bricks with three lives; each brick earns 10 points. Back returns to Settings, Pause/Resume freezes play, and New starts a fresh round. Completed rounds offer New to play again.

Detections and watched-device alerts continue through the existing scanning loop. An alert pauses the game; dismissing or timing out returns to the same round, still paused until you choose Resume. Locking or screen timeout also pauses. Leaving/reopening retains the round in RAM; rebooting loses the round. There is no persistent high-score file or SD requirement. Labels are currently English.

The game uses the existing display buffer and a fixed-size physics state. It has no audio, external artwork or network dependencies. Updates use bounded 8 ms physics steps with at most five catch-up steps per loop; game redraws are limited to approximately 30 FPS outside screen transitions. Actual board performance while scanning remains to be measured.

Testing the added menu entry exposed a settings-list buffer overflow when section headings repeat. Capacity now covers a heading for every row, including the two optional tracking rows. Full-loop ASan/UBSan tests verify the fix.

See BREAKOUT-TEST-REPORT.md for historical v0.5 size and validation; TEST-REPORT.md covers the current image. No device has been flashed.

## v0.4 size audit and content design

Lossless repeated-row glyph encoding preserves all 646 glyphs and saves 5,458 bytes in the bitmap data before decoder overhead. Every normal/Japanese glyph is checked against the original source. See RESOURCE-REVIEW.md for measured whole-image savings and limits.

**Correction: the unnamed 128 KiB flash region is occupied by persistent BlackBox history.** Both OTA slots and all storage offsets remain unchanged. The proposed 64 KiB-per-slot increase was rejected because it would disable history. A build guard now checks this implicit reservation. No partition migration is required or included.

Games/remotes will share compiled engines and screen templates with optional SD content. See SD-CONTENT-DESIGN.md for the proposed bounded pack reader, fallbacks, storage ownership and implementation gates. Breakout was subsequently implemented in v0.5. Remote drivers and an SD pack loader remain planned. Existing fonts and features remain available without SD.

## Field tools and resource cleanup

Earlier drafts added WiFi Remote ID, conservative ExpressLRS setup-network clues, a four-pilot FPV pit board, optional own-equipment MAVLink readings, selected BTHome sensors, alert filters, accessibility settings, and preview translations. See [FIELD-GUIDE.md](FIELD-GUIDE.md) for configuration and limits.

Settings → Shutdown / Reboot drains pending records, synchronizes and unmounts microSD before displaying the result. Safe shutdown leaves the display powered; disconnect power after success. Write failures are displayed rather than treated as success.

Static Field Tools pages redraw only when needed. Font bitmap deduplication saves about 7.2 KiB in that representation; no existing features, full-screen buffers or pin assignments were removed or changed. See [RESOURCE-REVIEW.md](RESOURCE-REVIEW.md).

## Research upgrade

Proposals 25–32 now have a local experimental implementation. **Settings → Research Lab** provides bounded microSD evidence sessions, redacted or explicitly selected raw JSONL/CSV exports, pinned field annotations, scan profiles and coverage statistics, validated data-only signature imports with rollback, expanded Axon/Meta/Flock/Raven clues, and an honest other-ALPR support table. Optional resources include the map and public-records toolkit supplied by the user. Read [RESEARCH-GUIDE.md](RESEARCH-GUIDE.md) for operation, provenance and limitations.

No new camera model has been validated on physical hardware. Community signatures remain experimental. The new research files have limits and rotation; existing normal history is separate. Additional games, the OSRS font and full progression backup remain on the roadmap below.

## Included

- Alert lengths of **15, 30, 45 or 60 seconds**, default **30**, saved across restarts. Applies to full-screen detection/watch alerts and Desk detection cards.
- Eight queued alert snapshots, FIFO, with duplicate coalescing and an **X more** indicator. The current card is not replaced by new arrivals. Each displayed card gets a new timer. Overflow is counted; queued events older than two minutes expire. History is retained independently within its own bounded capacity.
- **Why this matched**, using the rule captured when the event matched: manufacturer prefix, WiFi name, BLE company/service/name, payload pattern or behavioral heuristic. Legacy records report unavailable evidence rather than inventing a rule. Confidence is a grade, not a probability.
- User-opened camera resources with readable URLs for ALPR Radar and DeFlock. Shared prefixes explicitly do not confirm a camera; Flock cards say **FLOCK MATCH**.
- Consistent encounter counts for WiFi and BLE, stronger-evidence upgrades, correct live totals when the 64-entry history wraps, and removal of duplicate Regulars bookkeeping. Lifetime-stat resets preserve live observations.
- A bounded, protected BLE observation queue so the Bluetooth callback no longer edits the history ring concurrently with the main task. The callback's temporary-string lifetime bug is also fixed.
- **Settings > DNSP Walkthrough**, four optional pages with Next/Skip.
- **Settings > microSD Status**: mount/read status, card class, FAT format, physical/volume capacity, used space/percentage and detected write failures. Volume label/name is explicitly unavailable in this driver; manufacturer/model identification is not implemented.
- Coalesced SD summaries of alert/BLE queue overflow, limited to one attempt per ten seconds, with two rotating files of approximately 32 KiB each. Failures do not recursively log themselves. This is not yet comprehensive error logging for every subsystem.
- Update-entry and squad-countdown warnings that upstream firmware replaces DNSP features; reinstall using a DNSP image from **your-dnsp**. Existing upstream signature verification is retained. The custom draft is identified separately while OTA/mesh comparisons retain the 1.19.1 baseline version.
- CSV radio-name fields are bounded, made single-line, stripped of CSV delimiters, and prevented from starting spreadsheet expressions.

## Still on the approved roadmap

OSRS-style font and RS3/OSRS content, Runecrafting and other games, further menu/time improvements, richer exports, full settings/progression backups, physical recovery validation, SD data packs, and further gift-readiness work. WiGLE remains deferred. The proposed whole-card duress erase and landscape/game landing screen are **not implemented**; the existing optional duress PIN remains.

Do not treat this draft as gift-ready. It has not run on a physical board. The card status/rotation/writes and 80 MHz display need real-device verification, including card absent/full/removed, crowded scanning, long runtime and reboot behavior. The new firmware/public-preferences backup and computer recovery route still require a physical recovery drill.

## Build and test

This is a buildable source snapshot, not a full Git checkout. Original code and attribution are preserved; see `LICENSE` (GPLv3). Some unrelated upstream tooling/docs are not part of the snapshot.

With Python and PlatformIO installed:

```sh
pio run -e cyd-fast
make -C test
sh test/run_dnsp_ui.sh
```

`cyd` is the same ST7789 board at normal display speed. `cyd-ili9341` is for a different display driver, and `awok` is another board; do not flash those images to this ST7789 device. The frozen 3.5-inch target was not tested.

Dependencies are pinned: Espressif32 6.5.0 (Arduino ESP32 2.0.14), TFT_eSPI 2.5.43, NimBLE-Arduino 2.5.1, and XPT2046_Touchscreen commit `f956c5d8ce3bf39169c7378416b89e7cfe70a034`. Do not set `SQW_VERSION` for an ordinary draft build; it is an upstream bench override.

The `DNSP_UI_TEST` emulator mode is only a desktop test fixture. Use the provided runner so it gets an isolated temporary settings directory. It does not connect to the physical board.

## Firmware files and installation

The accompanying firmware kit contains the application image and supporting boot/partition images with SHA-256 checksums and an offset manifest. It is an **unsigned local USB build**, not an upstream-signed OTA package. Do not upload it to the normal upstream OTA service or flash the application image at address zero.

The simplest source-based USB installation, when you choose to test on hardware, is:

```sh
pio run -e cyd-fast -t upload --upload-port YOUR_SERIAL_PORT
```

This invokes the board's normal multi-image upload using the included partition layout. It writes firmware, so keep a known-working firmware/recovery route before using it. No upload or erase command has been run as part of preparing this draft.

## Resources

The optional camera page points users to [ALPR Radar](https://alprradar.com/) and [DeFlock](https://deflock.org/) on another device. Radio matching does not visually identify a camera or establish its operator.
