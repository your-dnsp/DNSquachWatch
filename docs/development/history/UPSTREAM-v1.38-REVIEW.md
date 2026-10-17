# Upstream v1.38 integration for DNSP v1.9.1

Baseline: v1.37.0, f9547719353fdbf624c77703f2b816f49d8e6a40.
Integrated: v1.38.0, 65d971072bddef860318681922ba3102661378e4.

## Integration and conflicts

Keep DNSP's menu pages and main-loop dispatcher; add Fonts under Display & Appearance and return from Fonts to its parent. Add saved 12/24-hour clock format to Desk, zone and corner clocks. Font controls preserve existing language/large-text accessibility and the custom tag editor. A portrait preview at 2x shows one complete sample tile rather than clipping the second.

Retain DNSP's claimed-transmitter DEAUTH, BLE byte order, strict decoded Remote ID, research recording, simulation provenance, Ignore/Snooze handling, location labels, Wi-Fi connection-only operation, backup, recovery, security and signed DNSP OTA. The SD controller, write-response, and software-touch adapters are unchanged. No ILI9341 receiver, LoRa, GPS or WiGLE additions.

Integrate signature-table and payload additions while retaining DNSP's cautious Axon/Meta labels and existing research rules. Find Hub checks its service-data frame type, not every FEAA beacon. Add service-data UUID matching, FS accessory names, body-camera payload tags, tracker services and manufacturer entries. Append BLE_PAYLOAD evidence without renumbering old evidence or changing record layout. A body-camera tag is not a recording-state signal. Generic drone and gunshot-sensor help now covers the expanded signatures without claiming every drone match is decoded Remote ID or excluding ShotSpotter categorically.

Preserve DNSP's transient pet bitmap, embedded visuals and splash attribution. Include new companion arms, gestures, dance and bubble placement. Fixes already present in v1.9.0, including full-width tag Cancel/Back and the six-byte MEATY console prefix, remain.

## Size and storage safety

Replace the general scanner used for known integer/string formats with a small bounded scanner. Oracle tests compare signed/unsigned numbers, EOF/partial assignments, bounded IDs, every checkpoint truncation, legacy journals and pinned timezone formats against scanf. Only reviewed ASCII scansets, integer conversions and consumed-byte counters are supported; unbounded strings and floating conversions are rejected.

Keep all timezone calculations and DST-rule handling in newlib. Adapt only the scanner symbol in a build-local copy of the exact pinned timezone object and the existing coexistence version-query object; reject changed object hashes. Installed SDK/toolchain archives remain unchanged. The adapter test verifies original-archive preservation and rejection of an unknown object. Classic 2.8-inch CYD's large-panel mode is compile-time false; the desktop simulator retains runtime panel sizes.

No app-slot, NVS, blackbox, duress or content-contract changes. DNSP Content/v1.5 stays compatible. Tests cover native detection/storage/mesh rules, new signatures, parser equivalence, rendering in both orientations, font sliders, persisted clock format, ordinary receiver UI, drawing bands and companion actions. Physical radio delivery, card timing, touch/animation responsiveness and timezone display need hardware confirmation for this release.
