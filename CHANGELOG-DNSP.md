# DNSquachWatch changelog

## v1.5.1

Corrects the v1.5 backup crash confirmed as a loopTask stack-canary failure. History preparation and copying now use separate stack frames; a bounded cursor reads captured flash history without repeated full rescans. Backup yields between steps. Fresh empty readable collections skip unnecessary legacy duplicate scans; existing and interrupted collections retain recovery checks.

DNSP reports that backup completed on the physical ST7789-80MHz device. Serial output was captured at 115200 baud with no panic in the supplied run. This verifies that tested device/card combination, not every card or hardware revision. Host tests cover 1,800 stored events, bounded reads, cancellation/retry, duplicate prevention, write failures and snapshot invalidation. ESP32 build and image-integrity checks passed.

Download the kit matching your display; all four flashing binaries are together. **Keep existing /DNSP Content/v1.5 unchanged.** No new card-content package is required. Normal flashing retains settings, PIN/duress state and onboard history. Do not erase or format for this correction. See INSTALL-CONTENT.md for commands.

v1.5 is superseded because backup could crash. Both ST7789 and ILI9341 80 MHz kits are available, each with all four flashing binaries. ILI9341 compilation, image integrity and board-bound signature checks passed; ILI9341 hardware validation is pending. Display starts at 80 MHz; flash bus is 40 MHz. Diagnostic serial is 115200 baud. ST7789 app image is 1,950,096 bytes (15,984 bytes free); ILI9341 is 1,950,176 bytes (15,904 bytes free), in the same 1,966,080-byte slot.

Repository cleanup: Linux test header fix, expanded installer/submission/content checks, DNSP bug-report form, accurate package inventory, separate upstream release history and a tagged release with checksummed assets.

## v1.5 — superseded

Saved Wi-Fi connects at ordinary boot independently of Update Check; selected networks can connect without requesting updates. Introduced DNSP-signed public-repository updates and separate v1.5 card content. Backup still had a main-task stack overflow; use v1.5.1.

Earlier DNSP features are described in README.md. Inherited upstream notes are kept separately under .github/upstream-release-notes and do not describe DNSP versions.
