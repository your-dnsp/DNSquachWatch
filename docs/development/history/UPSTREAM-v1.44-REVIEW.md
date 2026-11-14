# Upstream v1.44 review for DNSquachWatch v1.9.5

The starting point is the published DNSP branch at
`9ab1ce0e472845edd396d5767fcdeb8440cb6f0c`. The prior upstream reference is
`61a4143c93f81d86bce3b4a1b24a363f31516044` (v1.40.0). The reviewed target is
`a4303d5f00ee9f72be0b57f0f0db343eece61718` (v1.44.0).
This is a selective port of the v1.40.0–v1.44.0 delta. The integration used an
isolated checkout of published DNSP source. The previous checkout and release
tag are retained for source comparison and rollback.

| Upstream change | DNSP disposition |
|---|---|
| v1.41 passive Wi-Fi scans | Ported to the manual scan and the shared setup/update network picker. Saved-network discovery was already passive. Authentication and explicitly requested OTA retain their existing radio behavior. Imported the upstream scan guard. |
| v1.41 OUI correction | Ported the Arlo Technology prefixes and removed the misattributed Espressif prefix `BC:DD:C2`. Focused regression tests and DNSP's existing OUI tests cover the table. The upstream registry-fetch workflow was left out of the DNSP release workflow. |
| v1.41 Privacy Mode LOG panel | Adapted to DNSP Masked Mode and the shared LOG/SCAN action panels. Vendor and type text remain visible, advertised names follow the existing mask, and label/report targets keep the original observation. |
| v1.42 Human Sensing and v1.42.1 browser follow-up | Deferred. Sensing takes over the Wi-Fi radio and pauses detection. It needs separate work on DNSP's scan, Wi-Fi and recovery lifecycle. The v1.44 buffer allocation fix addresses a regression in that excluded feature. |
| v1.43 Terminal Mode | Deferred. It replaces the Boring Mode home screen and needs integration with DNSP's home navigation, alert and accessibility behavior. The current firmware has only 2,768 bytes free in its existing slot. |
| v1.44 BLE scan hints | Adapted to manual BLE scan rows and their Watch/Hunt panel. The parser uses bounded advertising fields and retains existing names and labels. Guesses are display hints and do not feed detection, identity, evidence, simulation, history or submissions. |
| v1.44 Hunt address handoff | Deferred. It guesses that a newly seen MAC belongs to the same phone, so it needs a separate review of consent, Hunt persistence and DNSP address provenance. |
| Other-board and site changes | C5 OTA core selection, Tab5 touch, small-board input/security changes, battery UI, LoRa messaging, GPS/WiGLE, browser/flasher changes and board-specific SD paths are outside the ST7789 receiver scope. |

## Validation

PlatformIO built the ST7789-80MHz `cyd-fast` image with the pinned toolchain.
The complete image is 1,963,312 bytes in a 1,966,080-byte app slot. Static RAM
is 124,480 bytes, up 16 from v1.9.3 in the linked build. Each of the 20 existing
raw BLE scan rows gains one byte. Payload parsing adds no allocation.

The native suite, focused BLE parser checks with AddressSanitizer and
UndefinedBehaviorSanitizer, and seven receiver simulator modes passed. The
simulator modes cover the DNSP UI, Snooze, fonts, companions, earlier upstream UI
banded display and a pixel comparison verifying SSID masking. Xtensa GCC 8.4.0 syntax/type and format checks of the five
changed firmware source files completed with existing TFT touch configuration
and localized format-string warnings. Source review followed callers through
the LOG/SCAN panels, Watch/Hunt and the shared Wi-Fi/OTA picker. The passive scan
check covers firmware scan call sites. RF behavior and the new labels have not
been verified on physical hardware.

Runtime SD/touch/coexistence/timezone tests, installer tests, and card-content
checks passed. No partition, stored schema, update signing policy, DNSP
navigation, research submission destination or card content was replaced.
Existing DNSP Content/v1.5 remains the runtime content version.
