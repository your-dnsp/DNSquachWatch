# DNSquachWatch v1.1.2 notes

This source snapshot targets the classic 2.8-inch **ST7789 CYD at an initial 80 MHz display clock** (`cyd-fast`). It uses SquachWatch v1.25.0 as its upstream base while preserving the full DNSP feature set and DNSP detection decisions.

Major additions include recovery-first boot, normal-menu microSD recovery, safer PIN and duress flows, comprehensive security-excluding backups, incremental readable BlackBox exports, stored scan/system history, adjustable radio profiles, applicable upstream improvements through v1.25.0, spam-flood suppression, Snooze Summary, dependable glitch disabling, user device labels, the Sketchy Environment rule, and the screen-light, randomizer, timer/counter, Pocket Reader, and Radio Activity tools.

The preceding v1.0 release candidate passed initial testing on the user's ST7789 device. The v1.1 labeling, Rules, history/export, backup, and profile additions still require a short physical regression pass. Nothing has been pushed to GitHub. See [README.md](README.md), [UPSTREAM-MERGE.md](UPSTREAM-MERGE.md), [RECOVERY.md](RECOVERY.md), [DURESS.md](DURESS.md), and [TEST-REPORT.md](TEST-REPORT.md).
