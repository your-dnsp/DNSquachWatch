# v1.5 validation

- Pinned PlatformIO ESP32 ST7789-80MHz compilation and the complete-image fit guard pass.
- The complete host test suite passes, including per-source DEAUTH, record/export/journal logic, storage retry, security/duress, labels, rendering helpers and malformed input checks.
- DNSP UI simulation passes in landscape, portrait, reboot and half-buffer/full-frame comparisons. Duress UI simulation also passed during this update.
- Five installer tests pass: ordinary updates preserve state; erase requires yes; decline/EOF cancel; damaged binaries cannot trigger erase.
- All 702 serialized translations, versioned card-file hashes, and 19,757 font bytes match the retained source fixtures. Every normal/Japanese font bitmap also matches when decoded from bounded 64-byte card windows.
- The signed update image is checked against the DNSP public key; altered images and another board's signed message are rejected before publication.
- USB kit image headers/digests, archive integrity and per-file SHA-256 checks are validated before delivery.

These are software/build checks. This v1.5 image has not been flashed by the assistant. Hardware testing remains required for long backups on busy/slow cards, saved home Wi-Fi join/time/location, both display panels, and a full signed network update. Avata 2 field validation and human translation review remain pending. No destructive device/card test was performed.
