DNSquachWatch v1.8.2 — ST7789-80MHz, SquachWatch base v1.35.0

microSD now owns HSPI and the display stays on VSPI. Resistive touch uses
software SPI on its existing pins; the pinned library's pressure, filtering,
rotation and calibration logic remains. Capacitive touch remains on I2C.

Probe v1.6 passed cold boot, two RST boots and repeated file reads on the
affected Lerdisk card with display active and sector writes blocked. This
full receiver build still needs hardware verification, including touch,
radios, card writes, backup and Safe Shutdown. It is not a guarantee for all
card batches. Firmware and card content should not be erased for this test.

TEST THIS BUILD
1. Flash all four binaries normally, retaining settings. Use your actual port.
2. Keep the same card and DNSP Content/v1.5 unchanged. Capture serial at 115200.
3. Check menu taps and walkthrough reads after boot, then after two RST cycles.
4. Complete a backup. Use Safe Shutdown and wait for safe-to-power-off confirmation.
5. Report mount errors, stalls, touch issues or resets with serial output.

Normal firmware writes are ENABLED; this is not the read-only probe.
Physical verification of this exact receiver image remains pending.
