> Historical v0.6 review. In v0.7, Four Favorites and Help & Recovery are additional main entries; see README-DNSP.md for new destinations.

# DNSquachWatch v0.6 — menu review

Local draft. No GitHub push or device flash. Existing functions retained; this release changes navigation, not detection rules.

## Findings and changes

The Settings list had interleaved repeated section headings; Breakout and fieldwork tools fell into the generic System group. Language and Accessibility required finding Field Tools' second page. Detailed alert controls occupied the first several main-menu rows. Shutdown was inside System. Group folding was shared across pages, which could unexpectedly hide another page's rows.

Settings now has 15 destinations in the mesh build, grouped into five contiguous sections. Alerts and games have their own pages. Language, Accessibility, microSD Status and Shutdown / Reboot are direct entries. Section headings show + or - to indicate folding. Scroll and folding state are separate for each page during a visit. Opening entries have arrows where previously missing. Field Tools reports a busy updater instead of silently ignoring the tap. Boring Mode's confirmation now correctly says it disables character activities, rather than claiming the rows disappear.

## Where to find things

| Settings destination | Available options |
|---|---|
| Alerts & Detection | Alert length (15/30/45/60 seconds), confidence filter, auto snooze, type filter, Evidence & Mutes, ignored devices |
| Field Tools | FPV pit board, drone readings, own telemetry, sensors; More retains alert rules, accessibility, language and help |
| Research Lab | Session/export controls, coverage, field notes, signature packs, device support, camera resources, public-records information |
| Appearance | Character size/outfit/pet/shades/banter/hat, theme, background/lock, brightness, display inversion/color order/rotation, status light |
| Accessibility | Contrast, reduced motion, left-handed field controls, larger shared controls |
| Language | Existing language previews; Hebrew remains behind the seven-tap title unlock |
| Desk Mode | Open desk, background, clock font/size/backdrop/time zone, mesh squad options |
| Games & Squachy | Breakout, Bingo, Dex, diary, Show Off, Replay Intro, Boring Mode |
| SquachMesh | Existing name and mesh controls |
| DNSP Walkthrough | Skippable introduction/help |
| microSD Status | Existing card/mount/capacity/usage information |
| Power Saver | Existing display/power preferences |
| Security | Existing PIN/privacy controls |
| System | Touch calibration, color check, diagnostics, firmware updates/checks, saved Wi-Fi networks, reset stats |
| Shutdown / Reboot | Safe shutdown, reboot, Back |

Active watch/hunt targets still appear at the top. Unearned character secrets retain their existing visibility rules. Unsupported firmware-update entries remain hidden. Boring Mode leaves disabled character options visible with their reason. Main-screen log/scan shortcuts and the existing mesh/security/research workflows are retained.

Back from a Settings subpage goes up to Settings; the main Settings Back leaves the menu. Direct Language/Accessibility/Evidence entries return to Settings; the original Field Tools paths retain their existing Back behavior. The research pages still use the existing numbered Next Page navigation; a research index could be a future improvement. No search bar or favorites were added.

## Validation

- 31 native unit suites passed.
- Three full UI runs passed: landscape, portrait, reboot path.
- Two full UI runs under AddressSanitizer and UndefinedBehaviorSanitizer passed, including screenshots in both orientations.
- Added checks cover direct shortcuts, Back, Alerts/Games entry, bottom-of-list reachability across all six Settings pages, and independent section folding. Existing tests still cover hidden Hebrew unlocking, Breakout/alert interruptions, PIN lock, and safe shutdown.
- CYD ST7789 80 MHz firmware build passed. This release did not rebuild the other board targets.
- Six flash-layout tests passed; generated partition bytes match v0.5.
- Actual firmware image: 1,825,632 bytes; change from v0.5: +624 bytes.
- Remaining application-image space: 140,448 bytes (137.2 KiB).
- Static RAM: 102,048 bytes; increase from v0.5: 64 bytes.

Simulator screenshots were inspected for legibility. Native touch coordinate rounding initially made new tests tap a section boundary; tests now tap inside the row. The hardware build caught an Arduino DISPLAY macro collision in a new internal enum name; it was renamed before the passing build.

## Remaining limits

Physical thumb usability and long-session behavior need the real device. Translation coverage remains partial; new category labels fall back to English. Large-control mode does not resize every legacy screen. This menu work does not implement deferred features such as firmware backup, OSRS font, or WiGLE. Firmware size and RAM measurements do not establish electrical stability at the experimental 80 MHz setting.
