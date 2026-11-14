DNSquachWatch v1.9.4 for ST7789-80MHz, based on a selective integration of SquachWatch v1.44.0.

- Manual Wi-Fi scans and network discovery listen for beacons without sending probes.
- Arlo signatures use the correct manufacturer prefixes. An unrelated Espressif prefix no longer triggers a camera match.
- Manual BLE scans show tentative hints for unnamed phones and accessories. Masked Mode keeps type and vendor labels readable on the LOG action panel.

Keep DNSP Content/v1.5 and flash normally to preserve settings. The complete image leaves 2,784 bytes free in the existing app slot. Software validation passed. Physical verification of this build is pending.
