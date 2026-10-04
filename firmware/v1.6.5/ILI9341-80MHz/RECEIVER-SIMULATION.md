# Receiver simulation labeling

This v1.6.5 receiver firmware recognizes the **DNSP test-address convention**: preserve the tested three-byte OUI and use exactly `00:00:00` for the final three decoded MAC bytes. The convention is neither globally reserved nor authenticated. Real hardware may use it, and a sender may spoof it. `SIMULATED` describes the address convention, not proof of a particular transmitter or trustworthy research.

## Detection and provenance

The shared byte predicate checks only bytes 3, 4 and 5 of the address responsible for a finding. It does not scan payloads, formatted MAC strings, or advertised names. BLE checks use the already-decoded advertiser address; existing address conversion is unchanged. Wi-Fi addresses keep their conventional byte order.

Normal classifications, signatures, confidence, user labels, tags, subtags and supporting evidence remain intact. A derived `SIMULATED` subtag is appended once without modifying user-owned label storage. This predicate never creates a detection or changes parsing, filtering, Ignore behavior, thresholds or cooldowns. The radio callbacks do not perform simulation formatting or allocate extra memory.

Wi-Fi observations preserve whether their matching address was the claimed transmitter or the BSSID. BLE observations record advertiser provenance. Accepted stronger evidence carries its provenance through merging and queued alerts; weaker observations cannot overwrite it. Legacy observations without recorded provenance are described as a matched address.

Deauthentication simulation status comes exclusively from the claimed transmitter used by the existing per-source tracker. A simulated receiver/destination or BSSID alone cannot mark an ordinary transmitter. The existing threshold remains six observed frames in the true 3,000 ms sliding window, with a 15,000 ms cooldown per source. Counts describe observed channel dwell periods, not all traffic transmitted by a network.

Sketchy Environment endpoints retain their own addresses and provenance. Two marked endpoints yield `SIMULATED`; one yields `CONTAINS SIMULATED EVIDENCE`. Details and incident exports identify the marked ALPR and/or deauth endpoint. Neither status raises confidence or bypasses the rule's ordinary timing and matching conditions.

## Persistence and exports

The BlackBox record retains its 64-byte layout. Simulation and address-role bits use previously unused flag bits. Old records remain readable; a zero suffix can be recognized from an old decoded MAC even without a stored flag. Existing legacy BLE record normalization is reused before evaluating that MAC; Wi-Fi byte order is unchanged.

Sketchy incidents use a new NVS key, `events-v153`, with fallback loading of the previous layout. Older endpoints receive generic matched-address provenance rather than an invented source role.

Metadata reaches alerts, detail explanations, stored scan/rule history, restored records, session logs, current-log backup CSV, user-label findings, readable history, research sessions, device research reports and Drone Watch Remote ID captures. Research sessions use schema 3, device research reports use `dnsp-device-research-v2`, and Remote ID captures use schema 2. Report checkers continue accepting v1 device reports and require consistent simulation metadata in v2 reports. Redaction derives simulation status before masking the MAC, so REDACTED reports retain it.

Readable history writes `ALERT-HISTORY-v1.5.3.csv`; previous CSV files are retained with their original columns. User-label findings similarly use `user-labeled-device-findings-v2.csv`. A bounded, retryable migration backfills retained flash records when the new readable CSV/schema marker is absent. Previously rendered simulated text rows receive an annotated revision with a distinct record identifier; the original line can remain alongside it. The completion marker is written only after refresh finishes. This avoids silently skipping metadata after interruption and does not load all logs into RAM.

## Resource impact

The subsequent [size audit](SIZE-AUDIT-v1.5.3.md) removes production-only inclusion of the developer drawing benchmark and corrects report wording. Its final images are smaller than the initial simulation build figures below. No microSD content changes are needed.

ESP32 compile-time size checks confirm that `Detection` remains 68 bytes, `SketchyRule::Endpoint` remains 56 bytes, and the persisted BlackBox detection remains 64 bytes. Address provenance uses existing padding/flag space. Eight Wi-Fi queue entries gain one byte each (+8 bytes); research statistics gain a four-byte simulated counter. Deauth tracking and its queue are unchanged. No unbounded container or new per-device heap allocation is introduced.

Temporary formatting buffers grow by 180 bytes for research JSON/CSV together, 80 bytes for SD log formatting, 256 bytes for incident formatting, and 192 bytes for maximum-size Remote ID capture formatting. These are individual buffer changes, not a measured peak stack or heap figure. Full hardware stack/heap behavior still requires validation.

Final complete images: ST7789 1,965,936 bytes (144 bytes free); ILI9341 1,965,984 bytes (96 bytes free), against a 1,966,080-byte app slot. The baseline ST7789 image was 1,962,752 bytes, so this revision adds 3,184 bytes. Static RAM is 123,664 bytes (+12 bytes). Both builds pass the complete-image partition guard. Both supported CYD targets are built with their existing partition layout and 80 MHz default display clock. The application slot has very little remaining headroom; this change does not enlarge it or add unrelated features.

## Validation and hardware work remaining

Focused host checks cover exact suffix and near misses, unrelated payload zeros, unchanged ordinary classification/confidence, unknown zero-suffix input not becoming a detection, subtag preservation/deduplication, merging and queued provenance, storage restoration, legacy records, readable migration and redaction. Deauth checks distinguish source from destination/BSSID and retain burst/cooldown behavior. Incident checks cover fully simulated and mixed evidence, persistence and endpoint exports. Maximum-size research and Remote ID captures are checked for bounded formatting.

The full host suite and UI simulator are also run. Simulator screenshots review the simulation alert badge and mixed-incident explanation. Builds do not establish RF or hardware correctness. Still verify live BLE and Wi-Fi profiles from the separate tester, Remote ID observations, deauth bursts across channel hops, mixed incidents, reboot-restored history, and a real SD backup/export with redacted reports. The tester firmware itself is outside this change. PIN destructive/security tests remain deferred until a spare unit is available.

Final validation: the full `make -C test -j2` suite passed; `DNSP_UI_TEST=1` simulator checks passed, including both simulation screenshots; installer/submission tests passed, including v1 compatibility and v2 redacted simulation metadata validation; the three display-target and eight flash-layout checks passed. Both `cyd-fast` and `cyd-ili9341-fast` PlatformIO builds passed. No hardware test was performed for this receiver revision.

## Changed source areas

- Shared metadata: `include/simulation.h`, `src/simulation.cpp`, `include/state.h`, `include/detection_record.h`, `include/alert_queue.h`.
- Address provenance: `include/detection.h`, `src/detection.cpp`, simulator detection shim. No BLE identification algorithm changes.
- Persistence and rules: BlackBox header/source, SketchyRule header/source, readable logs, backup CSV and SD session logs.
- Presentation: detection explanations, language explanation routing, alert/rule alert, history/field tools, user-label UI and label-target wiring in main.
- Research/export: research session header/source/storage/report, device report generator, user-label target/storage, Drone Watch capture, report checker and issue guard.
- Verification: detection, deauth, BlackBox, SketchyRule, readable storage, research, research submission, Drone Watch and installer tests; host/simulator build lists and simulator review checks.
- Documentation: README, contribution guide, documentation index and this report.

Existing unrelated local changes from the preceding gift-readiness work remain in the workspace. This report describes the receiver simulation addition; no commit or GitHub publication was made by this task.
