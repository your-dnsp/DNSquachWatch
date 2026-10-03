# DNSquachWatch changelog

## v1.5.1

Corrects the v1.5 backup crash confirmed as a loopTask stack-canary failure. History preparation and copying now use separate stack frames; a bounded cursor reads captured flash history without repeated full rescans. Backup yields between steps. Fresh empty readable collections skip unnecessary legacy duplicate scans; existing and interrupted collections retain recovery checks.

DNSP reports that backup completed on the physical ST7789-80MHz device. Serial output was captured at 115200 baud with no panic in the supplied run. This verifies that tested device/card combination, not every card or hardware revision. Host tests cover 1,800 stored events, bounded reads, cancellation/retry, duplicate prevention, write failures and snapshot invalidation. ESP32 build and image-integrity checks passed.

Download the kit matching your display; all four flashing binaries are together. **Keep existing /DNSP Content/v1.5 unchanged.** No new card-content package is required. Normal flashing retains settings, PIN/duress state and onboard history. Do not erase or format for this correction. See INSTALL-CONTENT.md for commands.

v1.5 is superseded because backup could crash. Both ST7789 and ILI9341 80 MHz kits are available, each with all four flashing binaries. ILI9341 compilation, image integrity and board-bound signature checks passed; ILI9341 hardware validation is pending. Display starts at 80 MHz; flash bus is 40 MHz. Diagnostic serial is 115200 baud. ST7789 app image is 1,950,096 bytes (15,984 bytes free); ILI9341 is 1,950,176 bytes (15,904 bytes free), in the same 1,966,080-byte slot.

Repository cleanup: Linux test header fix, expanded installer/submission/content checks, DNSP bug-report form, accurate package inventory, separate upstream release history and a tagged release with checksummed assets.

## v1.5 — superseded

Saved Wi-Fi connects at ordinary boot independently of Update Check; selected networks can connect without requesting updates. Introduced DNSP-signed public-repository updates and separate v1.5 card content. Backup still had a main-task stack overflow; use v1.5.1.

The current feature overview is in README.md; earlier milestones are recorded below. Inherited upstream notes are kept separately under .github/upstream-release-notes and do not describe DNSP versions.

### Splash-label correction — 2026-10-02

Both v1.5.1 display kits and signed update images were rebuilt to show **DNSP v1.5.1 | base v1.27.0** on boot. The splash now reads the build version instead of a hard-coded label. The corrected kits replace the earlier same-version downloads; SHA256 checksums have changed. Detection, backup behavior, settings and card content are unchanged. An already-flashed device keeps its old splash label until the corrected image is flashed.

## Earlier feature milestones

The following notes preserve the former README release history. They describe changes at the time of each version; use v1.5.1 and the current README for present behavior.

## Included from v1.5

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
