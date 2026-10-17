# Upstream v1.39 integration for DNSP v1.9.2

Baseline: v1.38.0, 65d971072bddef860318681922ba3102661378e4.
Integrated: v1.39.0, 6997feeba55f2c54ccc391afd5aa6e261cd55d28.

## Applicable changes and conflict decisions

Remove Flipper Devices' 0C:FA:22 OUI and 0x0E29 company-ID matches, which upstream found were identifying the BUSY Bar desk sign. Keep Flipper Zero service UUIDs and name matching. Other DNSP research/signature paths remain unchanged.

Adopt twelve deauthentication/disassociation observations, or six all-FF destination broadcasts, from one claimed transmitter in a true inclusive 3000ms sliding window. Keep DNSP's independent 15000ms per-source cooldown, wrap-safe unsigned ages, deterministic eviction, protected-body handling, simulation attribution and Ignore/filtering. Do not import upstream's fixed bucket or global cooldown. A multicast destination does not qualify as broadcast. Counts remain observations made during channel dwell, not total network airtime.

Retain the last twelve samples in each of eight fixed source slots. For exact BSSID continuity, retain one full BSSID and a saturated consecutive-frame run count. This uses no probabilistic BSSID hash and remains correct for equal processing timestamps. Distinct receiver counts retain the existing bounded 16-bit hash approximation. Traverse samples chronologically so equal-time reason codes choose the last observation after ring wrap. No heap allocation, callback logging or record-layout change.

Disassociation uses the same bounded management-header parser and protected-reason safeguards. Research JSON keeps actual subtype 10 versus 12; existing CSV/schema fields remain compatible. DEAUTH details/report wording covers both management-frame kinds.

Merge companion sound-event identifiers and guarded sound recipes. They remain no-ops on the supported classic ST7789 CYD. Unsupported T-Deck/LoRa radar/chat, GPS/WiGLE and other-board UI changes are excluded; existing DNSP menus, state values, SD/touch adapters, security, locations, labels, research destination, embedded assets and signed DNSP updates are preserved.

## Validation and resource discipline

Focused tests cover 11/12 unicast and 5/6 broadcast thresholds, per-source isolation/cooldown, mixed traffic, inclusive window boundary, rollover, table pressure, simulation source provenance, disassociation bounds/export subtype, exact BSSID continuity including equal timestamps, latest reason after ring wrap, and a 30000-frame independent sliding-window oracle. Sanitizers check the changed tracker. Full native, simulator, driver-adapter, content, layout, installer and package checks accompany the release.

The first direct twelve-sample expansion exceeded static DRAM. The final compact BSSID representation and eight-source table avoid that regression. No partitions or card-content changes; DNSP Content/v1.5 stays compatible. Physical radio/card/touch behavior still needs hardware validation.

## Publication build portability

The official pinned macOS and Linux x86_64 timezone objects have different raw hashes but byte-identical allocated code/data and disassembly/relocations. Review of the Linux package (archive SHA-256 d356109e30ed9cbcfaba5fdee821b188fbc0781f5843aad490e20e9ac7b228dc) confirmed the same five scanner formats. The adapter explicitly permits both reviewed object hashes and still rejects every unknown object. Its test finds either the configured/local PlatformIO directory or the standard user installation so the CI check actually runs after toolchain installation. No runtime firmware bytes or version change is needed for this build-tool correction.
