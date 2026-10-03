# v1.5.3 size audit, including receiver simulation

Measured complete ESP32 images, 2026-10-03. No partition layout or microSD content changes.

| Target | Before audit | After audit | Remaining in 1,966,080-byte slot |
|---|---:|---:|---:|
| ST7789-80MHz | 1,965,936 | 1,959,696 | 6,384 |
| ILI9341-80MHz | 1,965,984 | 1,959,744 | 6,336 |

The net saving is **6,240 bytes per display**. Both final images pass the complete-image guard. Static RAM remains 123,664 bytes, including linker alignment. This is improved headroom, not sufficient room for unrestricted feature growth.

## Retained change

The developer-only `PRIM` drawing benchmark, primitive timing helpers and library-reference rendering self-check are now compiled only with the existing `BENCH_TOOLS` build flag. The source remains available for diagnostic builds. Normal rendering, backgrounds, Remington photo/star, menus, radio detection, backup/recovery, security, simulation metadata and normal serial health diagnostics are retained. The shipping console no longer accepts PRIM; it is not a user menu feature. Diagnostic builds must independently pass the image-size guard before flashing.

The audit also fixed ordinary research reports incorrectly including the SIMULATED convention explanation. That explanation is now conditional on the decoded MAC suffix, with a regression assertion. This correctness change adds 32 bytes on ST7789 and 16 bytes on ILI9341 before benchmark removal. Removing the benchmark then saves 6,272 and 6,256 bytes respectively; the final net saving is the same on both boards.

## Why simulation is not a confidence level

The predicate checks three decoded address bytes and uses existing spare flags/padding. Direct named simulation helper code in the initial ST7789 image totals approximately 287 bytes, excluding callers, strings and export/UI changes. The complete simulation feature added 3,184 bytes relative to the preceding ST7789 build because it also versions exports, retains provenance, migrates readable history, and displays endpoint status. Replacing the LOW/MED/HIGH grade would not eliminate those requirements or their cost.

Confidence describes the matching evidence. SIMULATED describes an unauthenticated DNSP test-address convention that real hardware can also use. Test fixtures should still exercise ordinary confidence, filtering, matching and alert behavior. Keep both fields. Combining their visual placement is possible but is not a meaningful size reduction by itself.

## Largest contributors and unsuccessful quick experiments

The baseline mapped symbol audit found Squachy's drawBody at 50,229 bytes, loop at 49,055 bytes (including inlined helpers), snowfall at 15,135 bytes, starfield at 10,197 bytes and the built-in Remington compressed photo at 9,608 bytes. Individual formatted-I/O routines occupy roughly 8–12 KiB each, with integer and floating variants retained by application and SDK dependencies. These symbol sizes are not independently recoverable savings and exclude much unnamed literal data.

Link-time optimization and removal of the unused TLS-server handshake remain enabled. A link-time small-helper inlining restriction produced no saving. Forcing the existing wide-line helper out of line produced no saving. Forcing shared color blending increased size by 48 bytes relative to the report-corrected baseline. Forcing shared Squachy scaling increased it by 304 bytes and exceeded the slot. All these experiments were reverted; normal rendering implementation and compiler settings are unchanged.

Do not blindly substitute integer-only formatting/scanning for the SDK's generic routines: radio/SDK callers and coordinate presentation need a separate call-site audit. Do not consume the BlackBox, crash or duress regions to enlarge app slots.

## Recommended next pass

If more space is needed, first audit diagnostic code and duplicated formatting helpers individually. Larger savings may require externalizing additional static help/explanation content with bounded reads, versioned checks and clear missing-card behavior. That would require a new microSD content package. None was moved in this audit. Remington remains built into firmware as requested.

Final validation: both 80 MHz CYD firmware builds and complete-image guards pass; the report redaction/formatting regression passes; UI simulator checks pass. Symbol inspection confirms the production image no longer contains runPrimBench, primTime or g_benchPrimNow. The complete host suite passed before this narrow audit; it was not redundantly rerun. No hardware validation or GitHub push was performed.
