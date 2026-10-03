# DNSquachWatch v1.5

DNSquachWatch is a friends-and-family modified edition of **SquachWatch** for the classic 2.8-inch ESP32 Cheap Yellow Display (CYD). It keeps Squachy, the original visual identity, the core detection model, and the spirit of the upstream project while adding device & microSD recovery, additional OUI and device research, labeling and OUI research contribution mechanisms, accessibility tools, security enhancements, additional scanning and logging capabilities, additional drone detection & support for FPV pilots, and whimsical additions.


**Label, tag, and contribute research:** open **Research & Data > Device Research**, then hold a LOG row to label/tag that individual device. **RESEARCH REPORT...** exports two files: submit only **REDACTED** and retain **PRIVATE** locally. The redacted copy keeps the observed prefix and replaces the device-specific MAC suffix with `XX:XX:XX`; advertised names are omitted. Paste its contents into [the GitHub research form](https://github.com/your-dnsp/DNSquachWatch/issues/new?template=device_research.yml). Labels and prefixes are leads, not proof that every device with that OUI is the same product. The repository is public. A GitHub account is needed to submit or comment. Reports start unverified; independent observations can corroborate them.

This release is based on **SquachWatch v1.27.0**. DNSP's decisions take priority where the projects differ, including the menu organization, conservative detection wording, storage workflow, recovery behavior, and custom tools.

DNSquachWatch is maintained by **dnsprincess**. Bugs and design choices in this modified firmware should be reported to DNSP and should not be attributed to the original SquachWatch creator.

## This release's hardware

The v1.5 primary release targets the **2.8-inch classic ESP32 CYD with ST7789**, with an initial **80 MHz display clock**. The ILI9341 80 MHz build will be published separately after the primary release. The System menu can switch the display bus to 40 MHz if a panel is unstable. The flash bus remains at 40 MHz.

Booting, scanning, essential recovery, and Remington remain built in. The complete walkthrough and installation document now require the supplied **microSD-content/DNSP Content/v1.5/** files on the card. Backups require the checked installation document. Flashing the ESP32 does not populate microSD. Firmware and microSD content are separate downloads for manual installation. Both are also provided in this repository. The optional offline installer can copy content and flash the board. No Wi-Fi content download is used. See [INSTALL-CONTENT.md](INSTALL-CONTENT.md) for the separate installation guide.


## Downloads

- [ST7789 80 MHz firmware kit](firmware/v1.5/ST7789-80MHz/DNSquachWatch-v1.5-ST7789-80MHz.zip)
- [microSD content](firmware/v1.5/DNSquachWatch-v1.5-microSD-content.zip) — copy the version-matched content; the on-card path is **DNSP Content/v1.5**.
- [ILI9341 80 MHz status](firmware/v1.5/ILI9341-80MHz/README.md)

If you are unsure which CYD display you have, try the ST7789 image. A solid white screen may mean the other controller is fitted: try ILI9341 when available. Keep all four flashing binaries from the same kit together. See [INSTALL-CONTENT.md](INSTALL-CONTENT.md) for flashing and manual card copying; source code is not required for flashing.

## New in v1.5

- Saved Wi-Fi is attempted at ordinary boot, including PIN-locked boots, regardless of **Update Check**. A successful connection attempts time sync and recalls a remembered network location. The radio then returns to detection; this is not a permanent Wi-Fi connection. Safe boot and persistent duress mode retain their radio restrictions.
- **Wi-Fi Networks > CONNECT** joins the selected saved network and synchronizes time without checking for updates. It also makes that network preferred at the next boot. Connection failures are visible; a timeout is not reported as a wrong password.
- Backup preparation uses separate bounded stack frames instead of combining large snapshots in the main task. Firmware completion is checked in small blocks. Backup progress and operation behind the PIN lock are retained.
- Updates use DNSP's public repository and a dedicated DNSP firmware signing key. They do not install the original creator's firmware. The board verifies the signature and matching display target before activating an image. HTTPS metadata is a notice, not authenticated research or a permission to install; downloads are authenticated by the image signature.
- Installer **--erase** is an optional recovery operation with a default-No confirmation. It removes PIN/duress state, saved Wi-Fi, preferences and onboard history. Card data is untouched, but microSD backups do not contain the security secrets needed to reconstruct those settings.
- Whole-program optimization retains the features and reserved flash regions while making room for the GitHub update transport.

### Included from v1.4

- **Remington stays built in:** neither his photograph nor shooting-star sprite is loaded from the card. Card content cannot replace them.
- **Complete card walkthrough:** larger-text pages cover location labels and research submissions early, followed by all major menus, security and recovery. Missing or corrupt content shows an installation message instead of breaking boot.
- **Automatic readable history:** enabled by default, adjustable under Storage & Recovery. It uses the existing interruption-safe journal and small background steps, without creating full firmware backups automatically.
- **Reliable storage feedback:** bounded finding-log retries, counted queue losses and visible storage warnings. Session filenames distinguish reboots/outings without needing Wi-Fi or a clock; rows include trusted epoch time when available and explicitly say when time is unset.
- **Long backups:** an authorized backup continues behind PIN auto-lock. Its percentage is visible without exposing findings; unlock to access or cancel it. History copying releases file handles between steps so normal logging can continue.
- **Research pairs:** clearly marked REDACTED and PRIVATE reports share one report ID. Only REDACTED is intended for submission. The offline checker and prepared GitHub workflow detect complete MAC addresses in pasted reports; they cannot prevent arbitrary attachments from reaching GitHub.
- **Masked Mode:** renamed from Privacy Mode. It masks supported screens only; logs, backups and Pocket Reader files are not anonymized. Charge Mode now explicitly directs users to Safe Shutdown before removing power or the card.
- **Upstream features retained:** compatible v1.26/v1.27 features, Legend aura, wizard outfit, terminal secrets, runtime history and bidirectional time zones remain. DNSP menu organization and detection rules retain priority.


## Retained from v1.2.1

Remington's Flying Toasters pass now lasts **seven seconds**. Three quick taps on the background summon him (each gap within 600 ms and all three within 1.2 seconds). Existing menu buttons keep their actions; use an open part of the background. Automatic appearances keep their varied 15–30-second gaps.

Backups now copy and read back each history file in **1 KiB steps**, keeping screen updates and cancellation responsive between storage calls. Progress shows elapsed time and file/firmware byte counts. A healthy backup may take more than five minutes; cancellation occurs after **two minutes without progress**, rather than five minutes total.

Readable history saves a checked cursor after each complete record and journals the pending record. Ordinary subsequent refreshes avoid rescanning all old text for every new event. Interrupted records are checked for duplicates before retrying. The first refresh of older-format history may take longer while it performs the compatibility checks. A fixed snapshot keeps the export head stable while scanning continues; if the flash ring overwrites a captured sector, the export fails visibly and can be retried. Logs already on the same microSD remain in place rather than being duplicated into every backup. Automatic readable refresh is incremental, not an independent backup of the whole card. Copy the card to a computer for protection against card loss or failure.

## New in v1.2

- Wi-Fi password entry repaints the keyboard correctly during banded screen rendering, addressing the black-screen behavior reported on hardware.
- System Credits now scrolls through the full text using the same controls as Update, including the independent ALPR researchers.
- **Alerts & Detections → Set Location** provides a manual 24-character label, Home/Work/Driving/Con presets and Clear. Each stored detection retains its original label; unset events say `no-label-set`.
- A label assigned while connected to authenticated Wi-Fi is remembered for that network. **Remember for verified Wi-Fi** also lets you associate a label after a successful connection earlier in the current boot. Scanning an SSID is insufficient. Clear forgets the relevant association. Manually selected labels survive disconnect for that session; automatically recalled labels survive the intentional boot radio release and remain visibly marked as recalled. A fresh boot starts unlabeled until a network recalls a label.
- Location metadata appears in detection history, research and Remote ID capture, readable exports, rule incidents and backups. Backups include the label dictionary and remembered network names, without Wi-Fi passwords. Location labels remain visible in research's redacted mode: avoid sensitive names when sharing files.
- **DNSP Tools → Remington**, the final item, shows the supplied dog photograph. Double-tap to return. Remington's approved pixel head also appears as a sparkling shooting star in Flying Toasters at varied 15–30-second intervals between passes; triple-tap an open area to summon him.

Location storage is deliberately bounded: 64 unique labels and six Wi-Fi associations. Existing labels remain reusable at capacity; new entries fail visibly instead of evicting historical label text. Allowed characters are letters, numbers, spaces, hyphens, dots and underscores; a label cannot begin with a hyphen. Old records without location metadata remain unlabeled.

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
- **System:** Masked Mode, Charge Mode, Last Run, display speed, diagnostics, Device Health export, credits, Safe Mode/recovery information, reboot, and Safe Shutdown.

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

Keep all four binary files from this kit together. Do not combine files from different firmware versions. Follow `DNSQUACHWATCH INSTALLATION.txt` for the exact offsets, backup location, erase choices, macOS/Linux commands, and recovery procedure.

The repository's older browser-flasher assets are not this release's installation path. Use the complete matching v1.5 kit. It contains all four binary images; no second source folder is required to flash it.

## Build and validation status

Dependencies remain pinned. The release build target is `cyd-fast`. The application slot is 1,966,080 bytes; the measured image size and remaining space are recorded in SIZE-AUDIT.md. The photograph uses lossless per-row compression; fonts use bounded lossless decoding. Neither needs another framebuffer or a heap-backed content cache. BlackBox detection records remain 64 bytes.

Host tests, simulator UI checks and the real ESP32 build are recorded in TEST-REPORT.md. These checks cannot establish hardware stability. Please validate Wi-Fi entry, credits scrolling, label recall, exports, microSD backup/restore and Remington on the physical device. Avata 2 field validation and human translation review remain pending. Source, contribution workflows, card content and the primary firmware are published together; see [firmware/v1.5](firmware/v1.5) for display-specific images.

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

## Gifts and support

**dnsprincess only gives out devices running DNSquachWatch as gifts. You cannot buy a prepared device kit from DNSP.** If you received one as a gift, you can reach out to DNSP for help or modifications.

The v1.5 card-content download also contains the extended font and translated text. English recovery text remains built in; missing or corrupt translation records fall back to English. Copy the version-matched content before using other languages.
