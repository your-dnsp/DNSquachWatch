# DNSquachWatch v1.6.5 — ILI9341, 80 MHz

This kit contains all four flashing binaries, their manifest and checksums, the optional offline installer, and recovery instructions. MicroSD recovery now pages its instructions, permits Back while tools are busy, and visibly reports card-test outcomes. Mounting verifies filesystem and root-directory access. Accessibility includes the shared Glitch Effects setting.

Keep existing `/DNSP Content/v1.5/`. No card-content update or ESP32 erase is required. Use the normal four-binary flashing command from this extracted folder; see RECOVERY.md and the repository installation guide. Serial monitoring uses 115200 baud. Flash bus remains 40 MHz; display defaults to 80 MHz.

Both display builds and focused recovery tests passed. The retained recovery implementation and backup were observed working on ST7789 with compatible cards. These exact images and ILI9341 require hardware confirmation; the incompatible green-card batch is not claimed fixed.

Application: 1,963,392 bytes; existing app slot: 1,966,080 bytes; remaining: 2,688 bytes. Partitions and stored formats remain compatible.

Original SquachWatch: Talking Sasquach / skizzophrenic. DNSquachWatch modifications: dnsprincess / DNSP. Research credit includes ReconGrunt, zmattmanz, Ringmast4r and rpriven. See the main repository for full credits and research submissions.
