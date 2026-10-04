# MicroSD recovery validation — v1.6.5

The release retains the recovery UI fixes, explicit card-test outcomes, mount/root validation and remount bookkeeping developed after v1.5.7. It uses the unchanged Arduino SD driver, classic CYD card pins and 4 MHz card clock. Temporary driver diagnostics, speculative CMD0 changes and standalone probe are excluded.

The working-card v1.6.4 hardware capture confirmed initialization with CRC enabled, FAT/root access, successful synchronization with zero recorded write errors, and remount. The user subsequently confirmed backup completion. Those observations validate the retained implementation; the final v1.6.5 binaries still require hardware confirmation. Multiple other cards worked; the new green-card batch failed even with independent slow software SPI. Its precise incompatibility is unknown and is not claimed fixed.

Native tests inject mount, FAT/root, temporary creation/write failures and successful read-back/cleanup, remount and shutdown. Focused recovery and Accessibility UI checks exercise both 320x240 and 240x320. Full landscape integration has 19 pre-existing failures, identical to v1.5.7; it is not claimed passing. Full portrait integration had 80 baseline failures on v1.5.7 outside these focused checks and is not claimed passing. PIN/duress testing on a spare unit, Avata 2 and human translation review remain pending.

Existing `/DNSP Content/v1.5/`, stored formats and ESP32 partition layout remain unchanged. No microSD content package is regenerated.
