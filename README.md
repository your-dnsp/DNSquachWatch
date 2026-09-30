# DNSquachWatch v1.1.2

DNSquachWatch is a friends-and-family modified edition of **SquachWatch** for the classic 2.8-inch ESP32 Cheap Yellow Display (CYD). It keeps Squachy, the original visual identity, the core detection model, and the spirit of the upstream project while adding DNSP recovery, research, accessibility, security, logging, FPV, field, and whimsical features.

This release is based on **SquachWatch v1.25.0**. DNSP decisions take priority where the projects differ, including the menu organization, conservative detection wording, storage workflow, recovery behavior, and custom tools.

DNSquachWatch is maintained by **dnsprincess**. Bugs and design choices in this modified firmware should be reported to DNSP and should not be attributed to the original SquachWatch creator.

## Choose the correct display image

The same-looking 2.8-inch CYD has shipped with two incompatible display controllers. There is no reliable way to identify the controller from the outside, so v1.1.2 provides two 80 MHz release kits:

| Release kit | Display controller | PlatformIO target |
|---|---|---|
| `DNSquachWatch-v1.1.2-ST7789-80MHz` | ST7789 | `cyd-fast` |
| `DNSquachWatch-v1.1.2-ILI9341-80MHz` | ILI9341 | `cyd-ili9341-fast` |

Try the ST7789 image first on the newer/common panel. If the display remains solid white, flash the ILI9341 image instead. A white screen after choosing the wrong driver ordinarily means the panel was not initialized; it does not mean the board was damaged.

Both images start the display bus at 80 MHz. The ESP32 flash bus remains at 40 MHz. A runtime option under **System** can change the display to 40 MHz if an individual panel flickers, shows torn pixels, dims irregularly, or is unstable at 80 MHz. The two builds otherwise contain the same DNSP features and data layout.

The firmware does not require a microSD card to flash, boot, scan, enter Safe Mode, or display its built-in help. A card is needed for exports, readable log copies, research files, backups, and card-based recovery material.

## What DNSquachWatch adds

### Detection and alerts

- Longer 15, 30, 45, and 60-second device popups, with 30 seconds as the default.
- A bounded popup queue, an **X more** indicator, and **Snooze All** for crowded environments.
- Qualitative confidence and **Why This Matched** explanations that distinguish a clue from proof.
- Expanded Flock, Raven, Axon, other ALPR, Meta, tracker, Remote ID, FPV, and ExpressLRS research coverage.
- Stored history for every retained alert event, with separate boot, crash, and system history.
- A **Sketchy Environment** rule that raises a second caution when an ALPR clue and a Wi-Fi deauthentication burst are observed within 90 seconds. The paired evidence is saved without claiming that the two events are causally connected.
- Per-source DEAUTH tracking. Six frames from unrelated claimed transmitters can no longer combine into one alert. Each claimed source has its own six-frame, three-second burst threshold and 15-second cooldown.
- DEAUTH evidence can retain the claimed transmitter, receiver targets, BSSID, reason code, protection bit, channel, signal, and observed burst details. The interface warns that 802.11 management addresses can be spoofed.
- Adjustable Stationary, Balanced, Fast Sweep, Maximum, and Custom scanning profiles. Maximum returns to Balanced after 30 minutes and after a reboot.

Radio matches remain observations rather than certain identification. Manufacturer prefixes can be shared by unrelated products, signal strength is not distance or direction, and Remote ID broadcasts are not authenticated. The ESP32 sees legacy BLE and 2.4 GHz Wi-Fi; it cannot receive 5.8 GHz FPV video or BLE Coded PHY. Channel hopping, radio coexistence, interference, and transmission timing can all cause a device to be missed.

### Research, history, and microSD

- Research sessions with Balanced, Bluetooth-only, and Wi-Fi-only profiles, visible elapsed time, activity counters, coverage information, redacted/raw choices, and explicit save progress.
- Incremental **Refresh Files** output with stable record identifiers and an interruption-safe journal.
- **Export & Organize** for a numbered, permanent, human-readable snapshot.
- Pocket Reader support for the generated text files.
- microSD status showing card type, filesystem, capacity, use, and mount health where the hardware and FAT layer expose them.
- Recovery controls in the ordinary **Storage & Recovery** menu and in Safe Mode: remount/find the card, test read/write, and a deliberately confirmed format operation.
- Verified backups containing the running application, complete readable history, current log, labels, Ignore entries, alert-rule state, radio profile, public preferences, and active Watch/Hunt targets.
- The installation guide is included in completed backups. PINs, Duress state, Wi-Fi credentials, and authentication secrets are deliberately excluded.

**Safe Shutdown** stops acquisition, drains pending Bluetooth, Wi-Fi, DEAUTH, normal event-log, BlackBox, Research, and Drone/Remote ID capture records, saves counters, synchronizes the card, and unmounts it. Wait for the confirmation screen before removing power or the card. Safe Shutdown does not automatically create a new **Refresh Files**, **Export & Organize**, or full firmware backup. A running full backup is canceled and remains incomplete without `COMPLETE.txt`.

### Security and recovery

- An optional interface PIN lock with clearer setup, removal, retry, and wipe confirmations.
- A separately configured Duress PIN that is accepted only from the lock screen.
- Duress enters the persistent **Pixel Tide** decoy and attempts bounded cleanup of DNSP-owned internal and microSD data.
- Duress cleanup is best effort. It is not encryption, secure whole-card erasure, or a guarantee against forensic recovery.
- Ordinary reflashing can leave NVS state, including Pixel Tide, intact. Follow the full erase/recovery instructions when recovering from Duress.
- Safe Mode can start after repeated short boots or qualifying crashes. Holding the touchscreen during startup also requests recovery startup, which is useful when normal touch navigation or optional services are failing.
- Crash information is retained as separate bounded records and can be exported after a later successful boot.
- Gift Preparation restores the requested touch and color setup flow and explains that installing ordinary upstream firmware replaces DNSP’s modifications.

The PIN protects the interface; it does not encrypt flash or the microSD card. Treat readable exports, backups, research captures, user labels, and device identifiers as private data.

### Interface, accessibility, and tools

- Landscape orientation defaults to the USB-C port on the left, with rotation lock enabled by default.
- A marked DNSP splash and detailed credits separate the custom firmware from upstream responsibility.
- A skippable walkthrough, contextual help, fuller troubleshooting, progress indicators, and clearer descriptions for average users.
- Optional high contrast, larger common controls, reduced motion, left-handed footer placement, Auto Brightness, runtime display speed, and a dependable switch that disables transition/glitch effects while retaining meaningful celebrations.
- English plus preview catalogs for Spanish, French, German, Japanese, Simplified Chinese, and hidden Hebrew. Hold the English language choice for three seconds to reveal Hebrew. Hebrew, Japanese, and Chinese use compatible bundled glyphs rather than the optional Latin fantasy style.
- Original Squachy features remain grouped together. DNSP tools, games, research, alerts, FPV features, storage, and system controls have their own clear menu destinations.
- DNSP tools include Screen Light and Morse, SOS/rainbow/caution light patterns, Coin Flip, Dowsing Rod, Timer and Counter, Pocket Reader, and Radio Activity.
- **Squach Snacks**, a whimsical Breakout game, runs while preserving the device’s alert behavior.

## Menu map

The principal destinations are:

- **Alerts & Detection:** enabled detection types, popup behavior, confidence/help, rules, stored alert history, Watch/Hunt, and detection troubleshooting.
- **Research:** timed research sessions, raw/redacted capture choice, reports, readable history, and research guidance.
- **FPV:** drone readings, Remote ID capture, pit/frequency planning, ELRS clues, and supported own-equipment telemetry.
- **Squachy:** the original mascot and inherited Squachy modules.
- **DNSP:** DNSP walkthrough, Squach Snacks, Pocket Reader, Timer and Counter, Screen Light/Morse, Coin Flip, Dowsing Rod, and Radio Activity.
- **Storage & Recovery:** microSD information and recovery, readable files, organized exports, backup/restore, crash reports, and safe storage guidance.
- **Appearance & Accessibility:** theme-related choices, Auto Brightness, transition effects, language, contrast, control sizing, motion, and layout options.
- **System:** display speed, diagnostics, Device Health export, credits, Safe Mode/recovery information, reboot, and Safe Shutdown.

Some labels move slightly with display orientation or enabled features, but tools stay in their subject area rather than a favorites-only duplicate.

## Installation and recovery material

Detailed flashing commands are intentionally kept out of this README while DNSP revises that section. Each release ZIP is self-contained and includes:

- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`
- `manifest.json`
- `SHA256SUMS`
- `flash-macos-linux.sh`
- `DNSQUACHWATCH INSTALLATION.txt`
- recovery, Duress, feature-status, research, size-audit, and test documents

Keep all four binary files from the same display kit together. Do not mix an ST7789 application image with ILI9341 boot/release files or combine files from different DNSquachWatch versions. Follow `DNSQUACHWATCH INSTALLATION.txt` for the exact offsets, backup location, erase choices, macOS/Linux commands, and recovery procedure.

The repository's older browser-flasher assets are not the v1.1.2 release path. Use one of the two complete v1.1.2 kits above until a DNSP web flasher is deliberately rebuilt and validated for this version.

The project uses two 1,966,080-byte application slots, preserves dedicated NVS, BlackBox, coredump, and Duress-journal regions, and does not use SPIFFS/LittleFS for ordinary storage. A microSD cannot be populated by `esptool`; optional card content must be copied separately or seeded later by the running firmware.

## Build and validation status

Dependencies are pinned in `platformio.ini`. The two release targets are:

```text
cyd-fast             ST7789, initial 80 MHz display clock
cyd-ili9341-fast     ILI9341, initial 80 MHz display clock
```

The v1.1.2 size work compresses the embedded installation guide and reduces the Pixel Tide texture without removing user-facing features. The ST7789 application image is 1,918,512 bytes, leaving 47,568 bytes in its application slot. The ILI9341 image is 1,918,608 bytes, leaving 47,472 bytes. A proposed parser replacement was measured and reverted because it increased the image after shared ESP32 framework routines were considered. Link-time optimization remains deferred.

Automated native, malformed-input, security, storage, layout, display, and UI tests are described in [TEST-REPORT.md](TEST-REPORT.md). Exact firmware-image validation and package hashes are included in each release kit. Hardware testing remains necessary for touch calibration, panel colors, 80 MHz electrical stability, real radio reception, card behavior, Duress cleanup, and long-duration operation. The ST7789 line has received the user’s ongoing physical testing; the newly produced v1.1.2 ILI9341 image requires testing on an older-panel board.

See [FEATURE-STATUS.md](FEATURE-STATUS.md), [RESEARCH-GUIDE.md](RESEARCH-GUIDE.md), [RECOVERY.md](RECOVERY.md), [DURESS.md](DURESS.md), [SIZE-AUDIT.md](SIZE-AUDIT.md), and [UPSTREAM-MERGE.md](UPSTREAM-MERGE.md) for the detailed status and limitations.

Deferred work includes physical Avata 2 validation, fluent human review of translated text, optional WiGLE integration, and further size work such as measured LTO. Deferred items are not presented as completed features.

## Credits and thanks

**Talking Sasquach** on YouTube, **skizzophrenic** on GitHub, created SquachWatch and did the work that made this entire project possible. The device concept, Squachy, core detector, interface, and continuing upstream improvements come from that project. Please give the original creator the credit and kudos they deserve. The original firmware and web flasher are available at [squachwatch.com](https://squachwatch.com/), and the upstream source is [skizzophrenic/SquachWatch-CYD](https://github.com/skizzophrenic/SquachWatch-CYD).

The expanded Flock and ALPR work benefited from four independent public projects:

- [ReconGrunt/FlipDeFlock](https://github.com/ReconGrunt/FlipDeFlock) informed conservative signature handling, visual-verification guidance, and offline research ideas.
- [zmattmanz/flock-detection](https://github.com/zmattmanz/flock-detection) supplied research leads for accessory names, supplier clues, and combined evidence.
- [Ringmast4r/FLOCK](https://github.com/Ringmast4r/FLOCK) deserves particular thanks for the collected Flock and OUI intelligence and its public map resource. That research materially improved DNSP coverage while the firmware continues to treat shared hardware prefixes as clues rather than proof.
- [rpriven/flock-public-records-toolkit](https://github.com/rpriven/flock-public-records-toolkit) provides the public-records resource referenced by the firmware.

Those projects remain independent. DNSquachWatch does not imply their endorsement and does not treat any third-party dataset as proof that a live radio belongs to a specific device.

Additional protocol references and third-party notices are preserved in the source tree and release documentation. Bundled font source and licenses remain with the corresponding language assets.

## License

DNSquachWatch retains the project’s **GNU General Public License v3.0** licensing. See [LICENSE](LICENSE). The upstream README is preserved as [README-UPSTREAM.md](README-UPSTREAM.md).
