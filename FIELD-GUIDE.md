# DNSquachWatch v0.3 field guide

Local experimental draft for CYD ST7789. No physical-device validation yet.

## FPV and drone tools

Open Settings → Field Tools. The pit board stores four pilot assignments. Tap the left third of a row to change band, the rest to change channel. Channel 0 is unassigned. `!` means assigned frequencies are less than 40 MHz apart: a planning hint, not measured interference or a guarantee of safe separation. Export overwrites `/dnsp-fpv-pit.csv` on microSD.

Drone Readings keeps four recent aircraft/source slots from supported BLE and WiFi Beacon/NAN Remote ID. It displays field ages and warnings for missing fixes, stale/repeated timestamps, implausible jumps or conflicting identifiers. Claims received over the air are not authenticated. It does not locate every drone, decode flight-control traffic, or receive video.

Exact ExpressLRS setup SSIDs are LOW-confidence equipment clues. They do not prove a drone is flying. Air65/Air65 II ELRS aircraft, RadioMaster radios and Fat Shark goggles do not automatically provide readable telemetry; model, firmware and configuration matter. The CYD cannot receive ordinary 5.8 GHz analog video. External receiver work is deferred.

## Optional own-equipment telemetry

Copy `dnsp-telemetry.example.txt` to `/dnsp-telemetry.txt` on microSD and replace every value for your own compatible MAVLink-over-WiFi network. All five keys are required and duplicates/unknown keys are rejected. File limit: 255 bytes. Password may be empty for an open network, otherwise 8–63 ASCII characters. `source` is the expected sender IPv4 address; `system` is the MAVLink system ID (1–255); `port` is the local UDP listening port.

Open Own Telemetry and explicitly connect. Scanning and mesh activity pause during this mode. Leaving or locking stops the connection. No flight-control commands are sent. Normal WiFi connection traffic is transmitted. Only selected source/system packets with supported messages and valid CRC are accepted; signed MAVLink frames are rejected because this build does not verify their signatures. CRC is not authentication. Battery, GPS and radio fields depend on the sender; radio RSSI is shown as a raw protocol value.

The configuration file contains your network password in plaintext: use a dedicated equipment network and remove the file when no longer needed. Runtime temporary credentials are cleared; this does not encrypt the SD card. Hardware interoperability remains untested.

## Selected sensors and alerts

My Sensors discovers supported unencrypted BTHome v2 temperature, humidity and battery advertisements. Select up to four devices. Readings older than 60 seconds are marked stale. Encrypted packets and unknown object layouts are rejected. No generic BLE pairing is provided.

Alert Rules can quiet manufacturer-only matches, require multiple independent evidence fields, or mute up to four selected devices. These controls affect ordinary popup presentation; detections still enter bounded history/research logging. Confidence is an evidence grade, not a probability or proof of identity.

## Accessibility and languages

Options include high contrast, reduced motion, larger common controls and a left-handed footer on Field Tools pages. Legacy screens are not all mirrored. English, Spanish, French, German, Japanese and Simplified Chinese are visible language choices. Tap the Language title seven times to reveal Hebrew; once selected it remains accessible. Hebrew, Japanese and Chinese use bundled bitmap glyphs independent of a Latin themed font.

Translations are previews needing fluent human review. Common settings, Field Tools, help and generic detection explanations are covered; legacy flavor/game dialogue, detailed diagnostics and some dynamic technical labels remain English. Hebrew layout supports the supplied unpointed catalog and preserves Latin/numeric runs, not arbitrary Unicode text shaping.

## Shutdown and reboot

Settings → System → Shutdown / Reboot stops acquisition, drains queued records and synchronizes/unmounts microSD. Wait for the success screen before removing power or the card. Safe shutdown keeps the screen powered because the board cannot cut its own supply. Reboot uses the same sequence. Failed writes produce an explicit unconfirmed result rather than automatic reboot. Card-full/removal faults and sudden physical power loss still require board tests; this is not a power-loss-proof filesystem.

## Protocol sources

- OpenDroneID: https://github.com/opendroneid/opendroneid-core-c
- ExpressLRS setup networks: https://www.expresslrs.org/quick-start/webui/
- ExpressLRS MAVLink: https://www.expresslrs.org/software/mavlink/
- MAVLink messages: https://mavlink.io/en/messages/common.html
- BTHome: https://bthome.io/format/

Beacon spam and Doom are not included. Beacon spam cannot reliably exempt nearby SquachWatches or guarantee interference with surveillance equipment. Espressif's existing Doom port requires external PSRAM absent from this CYD configuration; a smaller Doom-like game would be separate work: https://github.com/espressif/esp32-doom
