# v1.5.2 validation

Both classic ESP32 CYD targets (ST7789 and ILI9341, 80MHz) compiled successfully after integrating compatible upstream v1.28.0 changes. Complete image checks and package hashes passed; ST7789 1,956,848 bytes, ILI9341 1956880 bytes, against 1,966,080-byte app slots. OTA signatures were generated and verified with the existing DNSP key and their individual target names.

Fresh host-suite compilation and tests passed. Added label regressions cover compact storage under a 256-byte simulated write cap, write-failure rollback, reboot/network recall, legacy migration and corrupt compact-record fallback. Squad codec checks cover the seventeenth outfit and older receiver interpretation. Landscape, portrait, reboot and banded rendering UI suites passed, including connection-only controls/back navigation and saving the Home preset under a restricted write cap.

Physical Wi-Fi/router behavior, saved-label recall after upgrade, and v1.5.2 backup regression remain to be tested. The earlier reported hardware backup success is v1.5.1 ST7789, not proof of this release on either board. The newer mbedtls 3 host crypto test is included as an opt-in target but was not run; classic CYD uses mbedtls 2 and its code compiled. No ESP32-C5 target is shipped.

Card content remains v1.5 unchanged. Flash normally without erasing settings or formatting microSD.
