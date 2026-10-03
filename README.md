# DNSquachWatch v1.5.7

DNSquachWatch is **dnsprincess's friends-and-family edition of SquachWatch** for the classic 2.8-inch ESP32 Cheap Yellow Display (CYD), based on **SquachWatch v1.28.0**. It keeps Squachy, the original visual identity, and the spirit of the project while adding device & microSD recovery, additional OUI and device research, labeling and OUI research contribution mechanisms, accessibility tools, security enhancements, additional scanning and logging capabilities, additional drone detection & support for FPV pilots, and whimsical additions.

**SquachWatch was created by Talking Sasquach / skizzophrenic.** This edition builds on that work; DNSP is responsible for the modifications. For the original firmware, visit [squachwatch.com](https://squachwatch.com/) or [the upstream project](https://github.com/skizzophrenic/SquachWatch-CYD).

**Label devices and contribute research:** open **Research & Data > Device Research**, then hold a LOG row to label or tag an individual device. **RESEARCH REPORT...** creates **REDACTED** and **PRIVATE** copies. Keep PRIVATE locally and submit only REDACTED through [the research form](https://github.com/your-dnsp/DNSquachWatch/issues/new?template=device_research.yml). Reports start unverified; others can comment with corroborating observations. A personal label does not identify every device sharing its manufacturer prefix. See [the contribution guide](CONTRIBUTING.md).

## Downloads and hardware

| Your display | Firmware |
|---|---|
| ST7789, newer CYD batches | [ST7789-80MHz kit](firmware/v1.5.7/ST7789-80MHz/DNSquachWatch-v1.5.7-ST7789-80MHz.zip) |
| ILI9341, older CYD batches | [ILI9341-80MHz kit](firmware/v1.5.7/ILI9341-80MHz/DNSquachWatch-v1.5.7-ILI9341-80MHz.zip) |

Both kits target the **classic 2.8-inch ESP32 CYD** and start with an **80 MHz display clock**. System settings can switch the display to 40 MHz; the flash clock stays at 40 MHz. If you are unsure which controller your CYD has, try ST7789 first. A solid white screen may mean it needs ILI9341: try the other kit. Keep each kit's four flashing binaries together.

Also obtain the [microSD content](firmware/v1.5/DNSquachWatch-v1.5-microSD-content.zip) and copy **DNSP Content/v1.5/** to the card root. **Existing v1.5 card content remains compatible with v1.5.7.** Flashing the ESP32 does not copy files to microSD. The full walkthrough, extended language content, and installation document use the supplied card files; backups require the checked installation document. Boot, scanning, essential recovery, and Remington remain built in. No Wi-Fi content download is used.

Each firmware ZIP includes all four flashing binaries, image checksums, an optional offline installer, and recovery instructions. See [installation guide](docs/user/INSTALLATION.md) for installation and manual card copying, or [GitHub Releases](https://github.com/your-dnsp/DNSquachWatch/releases/tag/v1.5.7) for both kits and download checksums.

## Latest update

**v1.5.7 improves storage diagnostics and removes repeated card lookups.** Slow history steps now report the operation responsible for the longest delay, making physical-card stalls easier to investigate. Automatic refresh keeps its paced processing and interruption protections. Receiver simulation labeling remains, and device screens and new reports share one version source. Existing v1.5 card content remains compatible.

Both display builds and host/UI checks are validated before packaging. Physical-card smoothness remains subject to hardware testing; previously confirmed ST7789 backup completion was on v1.5.1. Full release details are in [changelog](CHANGELOG.md).

## What DNSquachWatch adds

These are the additions and refinements in DNSP's edition. The original Squachy features remain available in their own menu; compatible upstream improvements are retained alongside DNSP's menus, research rules, and recovery choices.

### Detection with evidence you can inspect

Expanded research covers **Flock, Raven, Axon, other ALPR clues, Meta glasses, trackers, Remote ID, and FPV/ExpressLRS equipment**. **Why This Matched**, qualitative confidence, and contextual help explain the observation rather than assigning a definite identity from one manufacturer prefix. ALPR guidance points users toward further information and independent verification.

Device popups can last **15, 30, 45, or 60 seconds**, with 30 seconds as the default. Queued alerts show **X more**, and **Snooze All** helps when sightings arrive back-to-back. Stored Alert History includes every retained detection event, rather than just special rule incidents; boot, crash, and system records have separate views.

Wi-Fi **DEAUTH** findings require six observed frames from the **same claimed transmitter** within three seconds, with a separate 15-second cooldown for each source. Evidence can include BSSID, receiver targets, reason code, protection status, channel, signal strength, and burst details. Addresses can be spoofed; this is an observation of repeated traffic, not identification of an attacker.

The **Sketchy Environment** rule adds a caution when an ALPR clue and a deauthentication burst occur within **90 seconds**. It retains the paired evidence and exports an incident record to microSD, without claiming that the events are causally connected.

Scanning profiles include **Stationary, Balanced, Fast Sweep, Maximum, and Custom**. Maximum returns to Balanced after 30 minutes and on reboot. Radio Activity provides a view of observed activity within the ESP32's reception limits.

### Device labels, location context, and research contributions

Give individual devices names and tags so repeated observations are easier to interpret. **Alerts & Detections > Set Location** adds a manual label of up to **24 characters**, with Home, Work, Driving, Con, and Clear choices. Events retain the location active when they were stored; unset locations say `no-label-set`.

A location assigned to verified Wi-Fi can be recalled when that saved network is joined again. The interface distinguishes manual labels from recalled labels. Recall uses the network name, so check it when travelling or encountering another network with the same name. Saved Wi-Fi is attempted at normal boot, including PIN-locked boots, to synchronize time and recall a label before returning the radio to detection. **Wi-Fi Networks > CONNECT** opens a separate connection screen without checking for updates. Firmware checks require the explicit update action.

Research sessions offer **Balanced, Bluetooth-only, and Wi-Fi-only** profiles, explicit RAW/redacted choices, elapsed time, activity and coverage information, and visible save progress. Exported findings can support independent research instead of disappearing with an on-screen popup.

Research reports come in paired **REDACTED / PRIVATE** files with a shared report ID. REDACTED keeps the observed prefix, replaces the device-specific MAC suffix with `XX:XX:XX`, and omits advertised names. **Location labels remain visible: avoid sensitive location names when sharing.** The offline checker and GitHub submission workflow flag complete MAC addresses in pasted reports; they cannot prevent arbitrary attachments from being uploaded. Submissions are unverified until supported by independent observations. See [contribution guide](CONTRIBUTING.md).

Receiver test observations use the **DNSP test-address convention**: the original manufacturer prefix followed by `00:00:00`. Matching detections retain their normal classification and receive a **SIMULATED** subtag, including in history and redacted research reports. This convention is neither reserved nor authenticated; real hardware can use the same suffix. Combined rules distinguish fully simulated incidents from incidents containing one simulated endpoint. See [receiver simulation notes](docs/development/RECEIVER-SIMULATION.md).

### Readable history, backups, and card recovery

**Refresh Files** incrementally maintains text copies of retained findings for **Pocket Reader**. Automatic refresh is enabled by default when microSD is mounted. **Export & Organize** creates a permanent numbered snapshot for later review. Session log filenames distinguish outings even without Wi-Fi or a clock; trusted time is included when available.

**Backup & Restore** saves the running firmware, retained readable history, current log, labels, Ignore entries, rule state, scanning profile, public preferences, and active Watch/Hunt targets. Completed backups include the installation guide and a **COMPLETE.txt** marker. Progress remains visible during long operations, and an authorized backup continues behind PIN auto-lock; unlock to access or cancel it. Files already on the same card are not redundantly copied into every backup, and PINs, Duress state, Wi-Fi passwords, and authentication secrets are excluded.

microSD information shows card type, filesystem, capacity, usage, and mount health where the card and filesystem expose them. **Storage & Recovery** and Safe Mode offer remount/find, read/write testing, and a deliberately confirmed format operation. Storage failures produce warnings and bounded retries. Crash records can be exported separately; Device Health exports report their destination.

**Safe Shutdown** stops acquisition, saves pending records, synchronizes microSD, and unmounts it. Wait for the safe-to-power-down confirmation. It does not create a new full backup or organized export; a running backup is canceled and remains incomplete. **Charge Mode** lowers activity and power use but does not replace Safe Shutdown. Keep a computer copy of the card: a backup on that same card cannot protect against losing or damaging it.

### PIN security, Duress, and recovery

An optional interface PIN includes clearer setup, removal, retry, and wipe confirmations. The separate **Duress PIN** works only at the lock screen, attempts cleanup of DNSP-owned internal and card data, and enters the persistent **Pixel Tide** decoy.

The PIN does **not** encrypt flash or microSD. Duress cleanup is best effort, not secure whole-card erasure or a guarantee against forensic recovery. Ordinary reflashing can preserve Pixel Tide and other settings; use [the recovery guide](docs/user/RECOVERY.md) when a full erase is needed. See [DURESS.md](docs/user/DURESS.md) before enabling it.

**Safe Mode** can be requested by holding the touchscreen through startup, or entered automatically after qualifying repeated short boots or crashes. Recovery remains available when optional services fail. **Gift Preparation** restores the touch/color setup flow. Updates use DNSP's repository and signatures bound to the correct display target.

### A friendlier interface, FPV tools, and some whimsy

The skippable **DNSP walkthrough** covers the menus, labels, research submissions, storage, and security. Contextual help and progress feedback explain what is happening. Menus group Alerts, Research, FPV, Squachy, DNSP tools, Storage & Recovery, Appearance, and System by purpose.

Appearance options include **Auto Brightness**, high contrast, larger common controls, reduced motion, left-handed footer placement, optional Latin fantasy-style fonts, and a switch for transition/glitch effects. Landscape orientation defaults to USB-C on the left with rotation lock on. English is built in; Spanish, French, German, Japanese, Simplified Chinese, and hidden Hebrew have preview catalogs awaiting human review. Hold the **English** language choice for three seconds to reveal Hebrew; Hebrew, Japanese, and Chinese use compatible glyphs.

The **FPV** menu gathers drone/Remote ID information and capture, pit/frequency planning, ExpressLRS clues, and supported own-equipment telemetry. These tools do not make non-Remote-ID aircraft universally identifiable.

**DNSP tools** include Pocket Reader, Timer & Counter, Coin Flip, Dowsing Rod, Screen Light, Morse, and SOS/rainbow/caution patterns. **Squach Snacks** adds a whimsical Breakout game. RuneScape-inspired touches and extra easter eggs sit alongside the original Squachy features.

**Remington** has his own photo viewer—double-tap to return—and crosses the Flying Toasters background as a sparkling shooting star. Three quick taps on an open part of that background summon him. His photo and sprite remain part of the firmware.

## Limits and validation

Radio matches are clues, not proof of a particular device or owner. Manufacturer prefixes can be shared, signal strength is not distance or direction, and received Remote ID is not authenticated. Counts reflect frames observed during channel dwell, not every packet on a network. The classic ESP32 receives legacy BLE and **2.4 GHz Wi-Fi**; it cannot receive **5.8 GHz FPV video or BLE Coded PHY**. Radio coexistence, timing, and interference can cause missed sightings.

**Masked Mode** hides identifiers on supported screens; it does not anonymize logs, backups, or Pocket Reader files. Treat those files as private data.

Both display builds and automated tests passed. ST7789 backup completion was confirmed on hardware; ILI9341 hardware checks, Avata 2 field testing, and human translation review remain pending. See [validation report](docs/validation/TEST-REPORT.md), [v1.5.1 validation](docs/validation/TEST-REPORT-v1.5.1.md), and [resource audit](docs/development/SIZE-AUDIT.md) for details. Build targets are `cyd-fast` and `cyd-ili9341-fast`.

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

DNSquachWatch retains the project’s **GNU General Public License v3.0** licensing. See [LICENSE](LICENSE).

## Gifts and support

**dnsprincess only gives out devices running DNSquachWatch as gifts. You cannot buy a prepared device kit from DNSP.** If you received one as a gift, you can reach out to DNSP for help or modifications.

The v1.5 card-content download also contains the extended font and translated text. English recovery text remains built in; missing or corrupt translation records fall back to English. Copy the version-matched content before using other languages.

## Finding your way around the project

| Location | Contents |
|---|---|
| [Documentation](docs/README.md) | User guides, research guidance, recovery, validation and translation material |
| [Firmware](firmware/README.md) | Complete, versioned flashing kits for both CYD display drivers |
| `microSD-content/` | Files to copy to your card, with an integrity manifest |
| `src/` and `include/` | Firmware implementation and headers |
| `config/` | Flash partition layouts |
| `tools/` | Build checks, offline installation, backup verification and research submission tools |
| `test/` and `sim/` | Regression tests and desktop UI simulator |
| `examples/` | Optional research and telemetry configuration templates |
| `assets/` | Original artwork and supporting asset sources |
| `ota/` | Signed, board-specific DNSP update images and manifests |

For source builds, see [Build instructions](docs/BUILD.md). For using a device, start with [Documentation](docs/README.md).
