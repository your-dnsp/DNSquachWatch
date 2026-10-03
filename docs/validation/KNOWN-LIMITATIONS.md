# Validation status and known limitations

v1.5.2 builds and host/UI regressions passed; Wi-Fi/label hardware confirmation is pending. ST7789 v1.5.1 boot and backup completion were confirmed on DNSP hardware. Automated checks do not substitute for field validation.

Still to validate or refine:

- ILI9341 display, touch and backup behavior on hardware.
- Backup restore, interruption/retry, PIN-lock endurance and different microSD cards.
- Saved Wi-Fi/router compatibility, time synchronization during connection-only startup, and signed OTA updates on real networks.
- DJI Avata 2 Remote ID in the field.
- Human review of translated strings.

Pixel Tide palette changes are deferred. See the [validation reports](TEST-REPORT-v1.5.1.md) and [current overview](../../README.md) for tested behavior and device limitations.
