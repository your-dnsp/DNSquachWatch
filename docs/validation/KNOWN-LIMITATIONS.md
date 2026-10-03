# Validation status and known limitations

v1.5.7 builds, all 53 host groups, display/UI checks and signed package checks passed. ST7789 hardware traces through v1.5.6 show improved responsiveness and successful simulated-alert dismissal, with individual microSD pauses still observed. v1.5.7 diagnostic measurements need hardware confirmation. ST7789 v1.5.1 backup completion was confirmed on DNSP hardware. Automated checks do not substitute for field validation.

Still to validate or refine:

- ILI9341 display, touch and backup behavior on hardware.
- Backup restore, interruption/retry, PIN-lock endurance and different microSD cards.
- Saved Wi-Fi/router compatibility, time synchronization during connection-only startup, and signed OTA updates on real networks.
- DJI Avata 2 Remote ID in the field.
- Human review of translated strings.

Pixel Tide palette changes are deferred. See the [validation reports](TEST-REPORT-v1.5.1.md) and [current overview](../../README.md) for tested behavior and device limitations.
