DNSquachWatch v1.8.4 — ST7789-80MHz, SquachWatch base v1.35.0

Removes the speech bubble only from the boot splash so TALKING SASQUACH
remains visible. Squachy's cameo and other-screen speech remain.
The footer reads: modified by dnsprincess (your-dnsp).
The next line retains DNSP and upstream base versions.

Retains v1.8.3 storage fixes and all DNSP features. DNSP reported Test Card,
backup, Safe Shutdown and reset/remount passed in v1.8.3 on the tested
hardware/card; this exact splash image still needs a visual check on-device.

Flash normally, without erasing ESP32 or changing microSD content.
Existing DNSP Content/v1.5 remains compatible. Confirm the creator name,
footer and version on boot. Normal firmware card writes are enabled.
Desktop UI and banded-rendering checks passed; the exact splash still needs an on-device visual check.
