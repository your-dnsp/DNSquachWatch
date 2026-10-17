DNSquachWatch v1.9.2 — ST7789-80MHz, SquachWatch base v1.39.0

Integrates applicable upstream v1.39 detection changes. The Flipper Devices
vendor prefix/company ID no longer identify a Flipper Zero by themselves;
Flipper service UUID and name matching remain.

DEAUTH now covers deauthentication and disassociation: twelve frames of either
kind, or six broadcast-destination frames, from the same claimed transmitter
in an inclusive three-second sliding window. Cooldown remains fifteen seconds
per source. Filtering, Ignore, simulation provenance and existing evidence
remain. An all-FF receiver is broadcast; other multicast addresses do not meet
the lower threshold. Addresses can be spoofed and observations cover only the
channel dwell periods. Counts retain the latest twelve observed frames.

Eight fixed transmitter slots use 1,088 bytes, 256 fewer than v1.9.1's tracker.
Oldest-seen eviction remains deterministic; sufficiently many changing sources
can evict an entry and its cooldown. BSSID continuity remains exact; distinct
target counts use the existing bounded hash approximation. Research JSON
retains actual subtype 10 or 12 without changing storage layouts or schemas.

Preserves DNSP labels/tags, OUI research and submissions, locations, Wi-Fi
connection-only behavior, signed updates, security, recovery, accessibility,
embedded visuals and ST7789-only receiver scope. Unsupported LoRa, GPS/WiGLE
and other-board features are excluded. No SD/touch runtime-adapter changes.

Complete firmware image: 1,961,632 bytes. App slot: 1,966,080 bytes.
Remaining: 4,448 bytes. Static RAM: 124,312 bytes (256 below v1.9.1).

Flash normally without erasing the ESP32. Existing DNSP Content/v1.5 is unchanged;
no new microSD content folder is needed. Serial: 115200 baud.

Validation: full native suites, changed-tracker sanitizer tests and a 30,000-frame
independent window oracle, simulator UI/rendering, SD/touch adapter checks,
version/content/layout/installer checks, image and package integrity. Physical
RF, microSD power/reset/write timing and animation/touch behavior still require
hardware validation.
