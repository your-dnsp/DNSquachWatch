# DNSquachWatch v1.1.2 — readiness work and remaining red flags

This is a development release, not a certified gift-ready product. Earlier release candidates were flashed and tested on the user’s ST7789 board; these exact v1.1.2 images still need physical regression, and the ILI9341 build needs its first older-panel test. Existing features are retained. Simulation and automated checks do not replace field testing.

## Eight approved suggestions

1. **Feature ledger:** FEATURE-STATUS.md distinguishes implemented, experimental, planned and deferred work. It corrects the impression that everything in older planning documents has shipped.
2. **Endurance:** a 12,000-loop accelerated mixed-screen simulator run, sanitizer checks and on-device health measurements/export are provided. ENDURANCE-CHECKLIST.md covers the actual hardware tests. Hardware endurance is still NOT RUN; no CYD serial port was visible.
3. **Backup/recovery:** running firmware is copied to new SD slots, read back and hashed before receiving a completion marker. Validated public settings, operational choices, labels, Ignore entries, rules, active Watch/Hunt targets, current LOG, and readable BlackBox history are included. PINs, Duress state and authentication secrets remain excluded. RECOVERY.md and a read-only computer checker cover a matching-kit USB recovery route. The expanded backup still needs real card fault and restore testing.
4. **Practice:** four clearly simulated cards explain weak/stronger clues, FPV/Remote ID and Meta limitations. They never call detection ingestion and do not alter history, research or progression. Tests confirm this separation.
5. **Detection-status help:** reception totals, filters, ignored devices, queue drops, alert overflow and hardware/protocol limits are explained under Why No Match. These are not claims of complete radio coverage or distance measurements.
6. **Menu cleanup:** Four Favorites and obsolete landing pages were removed by request. Features remain reachable through their task-based menus; see MENU-REVIEW.md.
7. **Gift preparation:** a non-destructive checklist reviews credentials, history, SD files, accessibility and testing. It links to WiFi/Security/Accessibility. The owner can preview and arm a DNSP hello for the next normal unlocked startup. Arming does not show it immediately, and it is consumed once. This is a fixed DNSP greeting, not a free-text message editor or automated privacy reset.
8. **Field report:** finished research sessions gain a readable, identifier-free text summary, with confidence counts, coverage, losses and limitations. Reports retain one previous copy. JSONL/CSV now include the captured qualitative confidence. Counts are observations, not deduplicated devices. Manual report export requires a finished session and card.

Work followed the recommended priority: ledger → endurance groundwork → recovery → practice, then the remaining help/favorites/gift/report work. Firmware changes were checked together after integration.

## Additional changes

Breakout's menu name remains familiar, but the court is now **Squach Snacks**: smiling forest snacks, a picnic paddle, an acorn-like ball and picnic completion text. These are procedural drawings using the existing framebuffer and palette; no bitmap pack, audio or second buffer was added. Physics, pause-on-alert and controls are unchanged.

New installations default remote update requests and automatic boot update checks to off. Explicitly saved choices remain respected. The upstream replacement warning mentions the current boot's backup-verification status and the recovery route. A success remembered in this boot does not guarantee the files still exist or remain unchanged; the computer checker rechecks them.

Research failures and size limits now account for the current and queued records dropped at termination. The existing SHA-256 implementation is shared by PIN hashing and backup verification and checked against a known vector. Shared menu routing avoids separate favorites behavior. Help text pages instead of silently clipping, and cached Care screens are invalidated for orientation/language changes and after transition effects.

DEAUTH alerts now require six frames from one claimed transmitter within a true sliding three-second window. Tracking and cooldowns are per source, bounded to 12 fixed entries, and old entries are deterministically reused. Research Mode decodes the relevant 802.11 fields in its exports and separately counts coherent bursts; the language remains cautious because transmitter addresses can be spoofed and channel hopping samples only part of the traffic.

## Red flags and what to do

| Priority | Concern | Action taken / next gate |
|---|---|---|
| High | Experimental matches can be mistaken for verified cameras, recording status or aircraft activity | Preserve qualitative uncertainty, add practice/status/report explanations. Validate models with owned/lawfully collected samples and negative controls before stronger claims. |
| High | Real hardware/card behavior and recovery remain unproven | Keep this a draft. Complete ENDURANCE-CHECKLIST.md and the physical USB recovery drill before gifting. |
| High | PIN lock is not encryption; raw research, readable exports, and comprehensive backups can contain private identifiers | Backups always exclude PINs, Duress state and authentication secrets, but intentionally retain device identifiers and labels. Treat them as private, and use a fresh card plus reviewed reset when handing off personal equipment. No anti-forensic claim. |
| Medium | SD writes and driver calls can stall despite small chunks | One 1 KiB copy operation per loop, timeout/cancel, read-back verification, explicit failures. Hardware full/removal/power-loss tests still required; no hard real-time guarantee. |
| Medium | Firmware space is finite | Preserve both OTA slots and BlackBox, reuse drawing/crypto/menu code, report final measured size. Do not reclaim the BlackBox region or promise unlimited content. |
| Medium | Radio coexistence means observations can be missed | Expose loss/coverage counters and describe limits. Compare scanning and gameplay on the actual board. Avoid “no camera nearby” claims. |
| Medium | Multi-key settings restoration can be interrupted by power loss | Validate both backup records before applying them, retain a restore journal and fixed source slot, and make retry idempotent. Hardware interruption testing remains required. |
| Medium | Legacy signed updater and custom local unsigned builds differ | Keep upstream signature verification intact; explain DNSP replacement and use matching-kit USB recovery. Hashes alone are not publisher authentication. |
| Medium | Translations and accessibility coverage are incomplete | Keep preview labels and honest fallback to English; get native-speaker and real-user review. Hidden Hebrew remains intact. |
| Medium | Expanding menus and cached drawing can break navigation | Test shortcut routing, Back, lower rows, all Settings pages, lock and rotation in both orientations. |

The 80 MHz display setting remains experimental. If a real board shows artifacts or instability, select the runtime 40 MHz display option and retest before blaming application code. No claim is made that software tests establish electrical stability.

See TEST-REPORT.md for measured size and exact checks. Historical v0.4/v0.5/v0.6 reports remain historical; they do not describe the current image size.
