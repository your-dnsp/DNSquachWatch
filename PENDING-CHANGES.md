# DNSquachWatch v1.1.2 remaining work

The v1.1.2 ST7789 80 MHz firmware is built and has passed the automated checks listed in TEST-REPORT.md. The following work depends on physical equipment or later user input:

- Flash this exact image and complete the physical regression checklist in TEST-REPORT.md.
- Interrupt Readable Log Refresh and Backup & Restore on expendable data, then confirm retry finishes without duplicated identified records or an ambiguous restore slot.
- Confirm restored Watch/Hunt targets survive the first reboot and return to normal session-only behavior after the following reboot.
- Exercise a full, removed, shortened, and failing microSD during readable export and backup.
- Validate the Sketchy Environment rule with ALPR then deauth, deauth then ALPR, and observations just inside and outside the 90-second boundary.
- Capture a controlled DEAUTH test in RAW Research Mode and confirm the decoded source, receiver, BSSID, reason/protection fields, per-source burst count, and redacted-address behavior.
- Continue field validation of Flock, Axon, other ALPR, Meta, ELRS, FPV, and related signatures against positive samples and unrelated negative controls.
- Retest DJI Avata 2 Remote ID when the aircraft is available.
- Obtain human review before treating the non-English language previews as complete translations.
- Run a longer 80 MHz display, touch, radio, and microSD soak before distributing devices as finished gifts.

WiGLE hardware integration, an OSRS font, additional RuneScape content, and other deferred ideas remain future work rather than v1.1.2 release claims.
