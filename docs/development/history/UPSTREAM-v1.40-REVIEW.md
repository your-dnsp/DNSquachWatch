# Upstream v1.40 integration for DNSP v1.9.3

Baseline v1.39.0: 6997feeba55f2c54ccc391afd5aa6e261cd55d28.
Reviewed v1.40.0: 61a4143c93f81d86bce3b4a1b24a363f31516044.

## Applicable changes

Integrate repeated-tap per-device Snooze (10 minutes, 30 minutes, one hour, four hours, then cycle). The alert stays open until three seconds after the last fresh tap; any fresh tap on the card advances the selection. Existing release-to-arm and debounce handling prevents a held finger repeating. Selected durations apply only to popup suppression: classification, scanning, evidence and logging continue. Preserve the permanent Ignore list, including its legacy BLE address-order compatibility, and DNSP’s separate ten-minute Snooze All feature. A full timer list visibly reports failure and keeps the alert open, rather than announcing a snooze that was not applied.

Use a fixed 32-address RAM table with a parallel deadline array, retaining expiry and rollover behavior without per-record alignment padding. Timed entries clear on restart and never write NVS. Do not import upstream’s hidden persistent ALWAYS-snooze store: DNSP already has visible, removable permanent Ignore. Existing ignore persistence and backup formats are unchanged; clearing/restoring Ignore also clears temporary timers.

Apply the portable theme.cpp font changes for fire-owl, snow-character and wolf speech: use the Pets font selection, shrink overwide lines and restore the caller’s font afterward. Keep DNSP’s visual assets and flying-toaster behavior intact. The Snooze rendering merges around DNSP’s Snooze All/WHY THIS MATCHED controls and simulation/privacy evidence.

## Deliberately excluded conflicts and other-board changes

Do not replace the DNSP main dispatcher, per-source DEAUTH, radio callback, saved-Wi-Fi flow, manual location context, label/tag submission routes, security, coordinated storage/shutdown, signed DNSP updater or SD/touch bus adapters. Upstream detection.cpp’s hosted-radio extraction and TAB5/NimBLE guards have no new classification rule for this board and are excluded. LoRa NodeInfo/name/channel/settings changes, GPS/WiGLE, TAB5 hardware, other-board frame-push fixes, larger-panel/density changes, web flasher/install telemetry, enclosures and unsupported board presets are outside the classic ST7789 receiver. Existing enum values and flash storage areas stay unchanged.

## Validation scope

Native tests cover each Snooze duration, exact expiry, independent sources, unsigned timer rollover, default API behavior, full capacity, renewal, reclaiming expired slots, explicit resume, permanent Ignore precedence and absence of a snooze NVS key. Receiver-loop tests cover repeated taps, taps away from the original button, cycle wrap, confirmation delay, held touch, full-list failure and rendering in both orientations. Existing UI, font, companion, drawing-band, detection/DEAUTH, research, security and storage regressions remain part of release validation. Physical touch/radio/card timing still requires hardware confirmation.
