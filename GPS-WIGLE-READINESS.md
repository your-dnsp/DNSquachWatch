# GPS and WiGLE readiness

DNSquachWatch v1.1.2 includes the tested, hardware-independent NMEA parser from the SquachWatch v1.25.0 work. It accepts multi-constellation GGA, RMC, and GSV sentences, keeps integer-precision coordinates, rejects bad checksums, and marks a location stale after five seconds. No CYD GPS transport is enabled yet because the current ST7789 hardware has no built-in receiver and an assumed pin assignment could interfere with the display, touch, or USB serial connection.

For a later external module, the transport layer should feed bytes into `Gnss::feed()` and expose a user-selected, board-verified UART pin pair. GPIO27 is already the ST7789 display MOSI pin in this build and must not be offered as a GPS pin. GPIO1/3 share the programming/console serial path. Any connector and voltage recommendation must be verified against the exact CYD revision and GPS breakout before it appears in the device UI.

The future WiGLE export contract is deliberately narrow:

- Export Wi-Fi infrastructure access points observed through beacon or probe-response frames, using the BSSID as the network identity.
- Never upload client probe requests, data-frame client addresses, or other non-infrastructure Wi-Fi devices.
- Write no positioned row without a fresh, checksum-valid GPS fix.
- Mark test/fake fixes and exclude them from normal upload files.
- Keep Bluetooth observations out of the WiGLE file by default. A separate local research export can be considered later.
- Explain that a BSSID and location are observations, not proof of ownership or intent.

Live GPS capture, a GPS settings screen, hardware pin selection, and WiGLE upload/export remain disabled until an external module and its wiring are tested on this exact board.
