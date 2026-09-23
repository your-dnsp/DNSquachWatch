# DNSP v0.7 — readiness work and remaining red flags

This is a local development draft, not a certified gift-ready release. No device has been flashed and nothing has been pushed to GitHub. Existing features are retained. The release is intended to make further testing and recovery easier, not to claim that simulation replaces field testing.

## Eight approved suggestions

1. **Feature ledger:** FEATURE-STATUS.md distinguishes implemented, experimental, planned and deferred work. It corrects the impression that everything in older planning documents has shipped.
2. **Endurance:** a 12,000-loop accelerated mixed-screen simulator run, sanitizer checks and on-device health measurements/export are provided. ENDURANCE-CHECKLIST.md covers the actual hardware tests. Hardware endurance is still NOT RUN; no CYD serial port was visible.
3. **Backup/recovery:** running firmware is copied to new SD slots, read back and hashed before receiving a completion marker. Public preferences have version/range/checksum validation and an explicit restore confirmation. RECOVERY.md and a read-only computer checker cover a matching-kit USB recovery route. Actual card fault tests and a physical recovery drill are still NOT RUN. This is not a complete settings/progression clone.
4. **Practice:** four clearly simulated cards explain weak/stronger clues, FPV/Remote ID and Meta limitations. They never call detection ingestion and do not alter history, research or progression. Tests confirm this separation.
5. **Detection-status help:** reception totals, filters, ignored devices, queue drops, alert overflow and hardware/protocol limits are explained under Why No Match. These are not claims of complete radio coverage or distance measurements.
6. **Favorites:** four configurable shortcuts appear at the top of Settings. Edit cycles through a small safe list of existing destinations; launch uses the same destination handler and gates as the normal menu. Saved choices survive reboot.
7. **Gift preparation:** a non-destructive checklist reviews credentials, history, SD files, accessibility and testing. It links to WiFi/Security/Accessibility. The owner can preview and arm a DNSP hello for the next normal unlocked startup. Arming does not show it immediately, and it is consumed once. This is a fixed DNSP greeting, not a free-text message editor or automated privacy reset.
8. **Field report:** finished research sessions gain a readable, identifier-free text summary, with confidence counts, coverage, losses and limitations. Reports retain one previous copy. JSONL/CSV now include the captured qualitative confidence. Counts are observations, not deduplicated devices. Manual report export requires a finished session and card.

Work followed the recommended priority: ledger → endurance groundwork → recovery → practice, then the remaining help/favorites/gift/report work. Firmware changes were checked together after integration.

## Additional changes

Breakout's menu name remains familiar, but the court is now **Squach Snacks**: smiling forest snacks, a picnic paddle, an acorn-like ball and picnic completion text. These are procedural drawings using the existing framebuffer and palette; no bitmap pack, audio or second buffer was added. Physics, pause-on-alert and controls are unchanged.

New installations default remote update requests and automatic boot update checks to off. Explicitly saved choices remain respected. The upstream replacement warning mentions the current boot's backup-verification status and the recovery route. A success remembered in this boot does not guarantee the files still exist or remain unchanged; the computer checker rechecks them.

Research failures and size limits now account for the current and queued records dropped at termination. The existing SHA-256 implementation is shared by PIN hashing and backup verification and checked against a known vector. Shared menu routing avoids separate favorites behavior. Help text pages instead of silently clipping, and cached Care screens are invalidated for orientation/language changes and after transition effects.

## Red flags and what to do

| Priority | Concern | Action taken / next gate |
|---|---|---|
| High | Experimental matches can be mistaken for verified cameras, recording status or aircraft activity | Preserve qualitative uncertainty, add practice/status/report explanations. Validate models with owned/lawfully collected samples and negative controls before stronger claims. |
| High | Real hardware/card behavior and recovery remain unproven | Keep this a draft. Complete ENDURANCE-CHECKLIST.md and the physical USB recovery drill before gifting. |
| High | PIN lock is not encryption; raw research and other stores can contain private identifiers | Public backups/report exclude secrets/identifiers; gifting checklist calls out all stores. Use a fresh card and reviewed reset when handing off personal equipment. No anti-forensic claim. |
| Medium | SD writes and driver calls can stall despite small chunks | One 1 KiB copy operation per loop, timeout/cancel, read-back verification, explicit failures. Hardware full/removal/power-loss tests still required; no hard real-time guarantee. |
| Medium | Firmware space is finite | Preserve both OTA slots and BlackBox, reuse drawing/crypto/menu code, report final measured size. Do not reclaim the BlackBox region or promise unlimited content. |
| Medium | Radio coexistence means observations can be missed | Expose loss/coverage counters and describe limits. Compare scanning and gameplay on the actual board. Avoid “no camera nearby” claims. |
| Medium | Multi-key settings restoration can be partial if power fails | Validate everything first, verify stored values, preserve the source file and require reboot. Retry interrupted restore; full transactional settings migration remains future work. |
| Medium | Legacy signed updater and custom local unsigned builds differ | Keep upstream signature verification intact; explain DNSP replacement and use matching-kit USB recovery. Hashes alone are not publisher authentication. |
| Medium | Translations and accessibility coverage are incomplete | Keep preview labels and honest fallback to English; get native-speaker and real-user review. Hidden Hebrew remains intact. |
| Medium | Expanding menus and cached drawing can break navigation | Test shortcut routing, Back, lower rows, all Settings pages, lock and rotation in both orientations. |

The 80 MHz display setting remains experimental. If the real board shows artifacts or instability, compare with the normal-speed ST7789 build before blaming application code. No claim is made that software tests establish electrical stability.

See TEST-REPORT.md for measured size and exact checks. Historical v0.4/v0.5/v0.6 reports remain historical; they do not describe the current image size.
