# Upstream v1.36/v1.37 integration for DNSP v1.9.0

Upstream baseline: v1.35.0, dfaf7e93e0fb4da33c1e19f95cd0b6aa29939fd4.
Integrated source: v1.37.0, f9547719353fdbf624c77703f2b816f49d8e6a40.

## Included

- Connectionless squad frames in the primary BLE advertisement, bounded retry/status reporting and appearance refresh for an existing peer.
- Crowd NONE retains squad traffic, badge and messages while hiding visitors.
- Preserve guessed-clock state through reset rather than presenting an untrusted estimate as synchronized time.
- Shared pet movement, Yeti dances and M3@ty, including the Snowfall igloo interaction and shapes.
- Integer background calculations and simultaneous RAW Watch/Hunt/Ignore indicators.
- Correct the upstream MEATY console prefix length (six bytes, not eight).

## Conflicts resolved for DNSP

Startup, main-loop menus, touch and frame pushing retain DNSP's SD/display controller separation, software resistive-touch transport, delayed SD mount, coordinated Safe Shutdown, PIN/duress, Charge Mode and Pocket Mode handling. DNSP's keyboard repaint and full-width tag-editor Cancel/Back fixes remain.

Detection conflicts preserve per-claimed-transmitter DEAUTH bursts, address provenance, BLE address ordering, research capture and the research/advertising consent gate. The simulation suffix remains exactly DD:DD:DD. Existing labels/tags, location context, stored history, readable exports and redaction are unchanged. Raven matching remains narrow; a vendor prefix alone is not deauthentication evidence.

The boot-only bubble remains absent and attribution stays visible. Existing built-in artwork and interactions are retained. SD driver/write-response and touch adapters are unchanged by the merge.

GPS/WiGLE/wardriving, LoRa and unsupported board/display support are not enabled. The receiver kit is classic 2.8-inch ST7789-80MHz only. DNSP signed GitHub update routing and the v1.5 card-content contract are retained.

## Resources and verification

The new pet raster is a transient 320-byte stack object during painting instead of permanently reserved RAM. Immutable string-pointer tables are const. Optional Xtensa branch-target padding is omitted to reduce code size without changing artwork, detection behavior or storage reservations. The partition table, NVS, blackbox and duress reservations remain unchanged.

Native tests include five upstream transport/appearance suites. Desktop checks cover full receiver UI, label-editor hit areas, recovery action visibility, splash attribution, both drawing bands, all four new companion actions and both Yeti dances. These checks do not establish physical radio delivery, card timing or touch reliability. Hardware verification remains required for this merged release.

Final complete image: 1,965,008 of 1,966,080 bytes (1,072 free). Static RAM: 124,496 bytes, 144 above v1.8.5. Compiler stack estimates for the transient raster paths are 384 bytes (mini) and 448 bytes (body); these are per-function estimates, not a measured total call-chain high-water mark.
