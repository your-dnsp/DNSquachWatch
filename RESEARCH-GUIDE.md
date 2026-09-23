# DNSquachWatch v0.3 research guide

This local draft implements proposals 25–32 as an experimental research layer. It keeps the user-selected CYD ST7789 **80 MHz display** target (`cyd-fast`). No firmware has been pushed or flashed. Physical-board validation is still required.

## Start a research session

Open **Settings → Research Lab**. Choose Balanced, Bluetooth only or WiFi only. The default export is redacted; selecting RAW requires a second confirmation. Start requires a mounted, writable microSD. A session ends after five minutes, at its storage limit, on Back, on a write failure, when leaving the research screen or when locked.

Research temporarily selects passive Bluetooth scanning, suppresses the mesh advertiser and puts WiFi into unassociated promiscuous operation. Bluetooth-only disables WiFi promiscuous reception; WiFi-only stops BLE scanning. The previous general scanning behavior resumes afterward. Research entry is refused while an update transport owns the radio. These controls have been reviewed and compiled, but receive-only behavior needs verification with an independent radio on real hardware; do not treat the draft as a tested RF-silence guarantee.

Two current-session files are written: `/dnsp-research.jsonl` and `/dnsp-research.csv`. Starting the next session rotates them to corresponding `.previous` files, replacing the older previous session. Copy any session you want to keep before starting another. Both current files together are bounded below approximately 1 MiB, with a separate allowance reserved inside that ceiling for the summary. Queue capacity is twelve records. Saving happens in the main loop; radio callbacks never write SD. Power loss can leave an incomplete last line or an unmatched CSV/JSONL record; there is no claim of transactional FAT recovery.

Coverage reports **enabled time**, channels visited, observations, saved rows, queue omissions and write failures. BLE and WiFi share one radio: enabled milliseconds are not measured RF airtime. Observations count callback events, not unique devices. A duplicate advertisement/scan response can appear more than once. No detection does not mean no camera.

## What gets recorded

JSONL records include schema and catalog versions, session and observation IDs, relative uptime, radio/address type, RSSI, channel, classification, supporting-field bits, rule ID and a separate human verdict. There is no GPS or UTC claim. The final JSONL summary records per-channel enabled milliseconds and losses.

RAW includes MAC addresses and the first **96 bytes** of the observed BLE AD stream or WiFi management frame, with original and captured lengths. WiFi data-frame contents are not recorded. The ESP-IDF FCS is removed before management-frame analysis. The Bluetooth bytes are the advertising data supplied by NimBLE, potentially including a scan response, not a complete over-the-air packet capture. These files are **not PCAP**.

Redacted research exports omit MACs, payloads, probe fingerprints and note text; they retain relative timing and classification. This choice only controls the research export. Existing normal firmware history/logging is separate and may still contain identifiers. The previous research session may also be RAW. History wipe includes the four research export files and clears queued research records; deletion is not forensic sanitization.

CSV contains only generated numeric/enum values and a hexadecimal MAC or `redacted`, so radio names and arbitrary notes cannot become spreadsheet formulas. Notes are included only in RAW JSONL after bounded sanitization.

## Field notebook

The notebook selects a saved observation by ID. **Older** cycles through the eight retained observations. The selected record stays pinned while you decide; new arrivals cannot silently change the record your verdict refers to. Choose a short preset note, then **Visually seen nearby**, **Suspected** or **False positive**. The annotation is a separate record referring to the observation ID; it never raises the automated confidence.

“Visually seen nearby” means exactly that. It does not prove the radio belongs to the object you saw. On a computer, add any longer narrative, manual coordinates, photographs or control-location details to a separate field record keyed by session/observation ID. Free-text entry, GPS and automatic geotagging are not provided on the board in this draft.

## Detection changes and evidence grades

- Axon/TASER: company `034d`, services `fc81`, `fe6b` and `fe6c`, and Axon-like names. Single fields remain low; company plus service is medium. The label is equipment, not a verified body-camera model or recording state.
- Meta: Luxottica `0d53` plus Meta `fd5f` in the same observed AD stream supports “possible glasses”; either alone remains low. Names can be imitated. Other inherited Meta/Snap company clues also remain low. No remote-disable or recording-state detection is claimed.
- Flock accessories: reported 128-bit service `e8ccbb38-9532-46a8-9fe5-1814df172e6f`, Penguin/external-battery names, and XUNTONG manufacturer data are considered together. A ten-digit numeric name requires the supplier clue. A `TN` followed by fourteen digits is recorded only inside that supplier's manufacturer block. These are experimental accessory clues, not proof of an ALPR camera.
- Flock WiFi: exact `Flock-` plus four or six hexadecimal digits, bare `Flock`, and reported `test_flck` names. Corroboration with the own-vendor prefix can reach medium. A name alone is not confirmation. Generic names such as `Flock-Guest` do not acquire the strict-format evidence grade.
- Raven: custom services `3100` through `3500` are collected; three distinct services can reach medium. Generic Device Information, Health Thermometer and Location/Navigation services do not identify Raven. No firmware version is inferred from these alone. Raven is an acoustic-sensor category, not ALPR or a synonym for ShotSpotter.
- Broad Motorola/Genetec ALPR-associated prefixes are downgraded to low. L6Q, Genetec AutoVu and other ALPR models are explicitly **not model-validated** in this release.
- Locally administered MAC addresses no longer count as manufacturer OUI assignments. Existing deliberate hacker-tool address patterns remain separate weak patterns. WiFi data-frame recipients no longer inherit the transmitter's measured RSSI.
- “Why this matched” displays the recorded research fields and rule ID. The character's former numerical confidence estimates are replaced with qualitative grades; those grades are not measured probabilities.

These changes do not constitute a fresh validation of every inherited signature. Legacy rules outside this targeted review retain their prior behavior, and even the retained vendor registrations need a reproducible IEEE registry audit before being described as freshly verified.

## Probe fingerprints

The recorder computes a DNSP-specific 32-bit FNV-1a fingerprint from complete probe-request information elements, excluding the SSID and all MAC addresses. It hashes IE tags, lengths and contents; vendor IEs include only the first four payload bytes. Incomplete or over-1024-byte frames produce no fingerprint. Identical fingerprints can belong to unrelated devices and hash collisions are possible. They are useful experimental class clues, not stable identities or confirmation of a camera.

This is an independent fingerprint format. **Do not paste FlipDeFlock fingerprint values into a DNSP pack**: their algorithm and ours are not asserted to be compatible. No prevalidated camera fingerprints ship. RAW exports contain the fingerprint even if only the first 96 frame bytes were saved.

## Data-only signature packs

Copy an ASCII, LF-newline file named `/dnsp-signatures.txt` to the card. Import it on the Signature Packs page while no session is running. Maximum: 4096 bytes, twelve rules. Invalid input leaves the current catalog unchanged. Successful import replaces the previously imported additions, retains the built-ins and enables one-level rollback. Reboot returns to built-ins; reimport manually. Imported rules cannot execute code, connect to a network or specify high confidence. Keep the imported file alongside your exports: source/date/label text is validated on import but not held in scarce radio RAM or embedded in every observation. The source/date are operator-provided provenance, not a verified endorsement; packs are not cryptographically signed.

Header:

```
DNSP-SIG|1|2026092301
```

Each following line has seven fields:

```
id|kind|value|type|label|https-source|YYYY-MM-DD
```

IDs are 1000–65535 and unique within the file. Types: 1 Flock, 2 Axon, 3 Meta, 7 ALPR. Kinds: `C` company (4 hex digits), `S` service (4 hex), `U` service (32 hex without hyphens), `N` BLE name substring, `W` WiFi SSID substring, `O` globally administered OUI (6 hex), `F` DNSP probe fingerprint (8 hex). Names are case-insensitive for imported substring matching. Maximum label 20 characters, value 32 characters and HTTPS source 96 characters. Rule text must not contain a pipe, control characters or embedded newlines. The date format is checked, but its historical accuracy is not verified.

Fingerprint imports are experimental class matches and cannot establish that a randomized address belongs to an earlier device. Built-in rules retain precedence where they already match. Combined exports flag that an imported rule also matched, but currently retain only one winning rule ID rather than every matching imported rule ID.

The included `dnsp-signatures.example.txt` is a **synthetic example**, not a new detection database. Replace it with reviewed rules before actual use.

## Sources reviewed and how they were used

- [ReconGrunt/FlipDeFlock](https://github.com/ReconGrunt/FlipDeFlock), especially its [signature guide](https://github.com/ReconGrunt/FlipDeFlock/blob/main/docs/signatures.md): informed stricter provisioning names, bounded offline packs, separation of visual confirmation from radio inference, redacted exports and probe-shape research. DNSP does not adopt its “Confirmed” label for an SSID, automatically learn identities or import its fingerprints.
- [zmattmanz/flock-detection](https://github.com/zmattmanz/flock-detection): supplied research leads for numeric battery names, supplier-bound serial patterns and tracking multiple service clues. DNSP does not adopt numeric certainty scores, classify a camera from signal strength, or identify Raven from generic standard services. Hardware-specific GPS and S3 features were not copied to the CYD.
- [Ringmast4r/FLOCK](https://github.com/Ringmast4r/FLOCK): used as an optional third-party map link. Its README lists November 2025 data freshness. No map dataset or claimed sharing relationships are bundled or used to raise a radio match's confidence.
- [rpriven/flock-public-records-toolkit](https://github.com/rpriven/flock-public-records-toolkit): linked as a user-opened template resource. No letters are generated or sent by the firmware, and jurisdiction-specific legal claims or response deadlines are not embedded. Users should check current local requirements.
- [Flock-You](https://github.com/colonelpanichacks/flock-you): reported firmware-derived accessory UUID and naming leads. The research remains model/firmware-dependent and unvalidated on our hardware.
- [Bluetooth SIG Assigned Numbers](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf): confirms manufacturer/service assignments; assignments alone do not validate a product model.
- [OUI-SPY Detector](https://github.com/colonelpanichacks/ouispy-detector): candidate Meta combination and Axon signals.
- [Ray-Ban FAQ](https://www.ray-ban.com/usa/c/frequently-asked-questions-ray-ban-meta-smart-glasses): capture can work without the companion-app connection. Bluetooth disconnection is not reliable camera protection.
- [Motorola L6Q specifications](https://www.motorolasolutions.com/en_us/video-security-access-control/license-plate-recognition-camera-systems/l6q-quick-deploy-lpr/l6q-details.html): establishes available radio interfaces, not a model-specific detectable signature.

Research links were reviewed during this draft; upstream branches are mutable. This implementation uses independently written matching/recording code and attributed research facts, not copied external firmware implementations.

## What still needs real devices

Verify actual microSD write/removal/full-card behavior, scanning transitions, receive-only behavior, antenna/radio coexistence, crowded captures, heap pressure, battery/power interruption and the experimental 80 MHz display. Collect positive and negative controls for each target family before claiming model validation. A quiet wired/cellular camera, an unadvertised BLE device or an unsupported radio mode can be missed.

## v0.3 field observations

Research exports now label ExpressLRS equipment clues (rule 105) and WiFi Remote ID (rule 106). These remain observations, not authenticated identities. See FIELD-GUIDE.md for per-aircraft freshness and quality warnings.
