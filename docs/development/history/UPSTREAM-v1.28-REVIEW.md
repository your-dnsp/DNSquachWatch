# Upstream v1.28.0 integration into DNSP v1.5.2

Compared the published v1.27.0 and v1.28.0 tags. Shared character drawing, aquarium integer math, the Over 9000 outfit and its unlock, five-bit squad outfit encoding, compatible crypto wrappers and simulator support were integrated. DNSP remains targeted to classic ESP32 CYD displays; the new ESP32-C5 board target, its board files, build environment and web flasher were not added.

Clock merge conflicts came from DNSP having removed unused pin/I2C bench commands; those remain removed, while the new AURA and OUTFIT commands were integrated. SD logging conflict was resolved in favor of DNSP session filenames, bounded retries, error handling and richer evidence logs. DNSP main.cpp received only the new console controls rather than wholesale replacement. DNSP security/duress, startup memory handling, menus, Remington, glitch setting, researched signatures, per-source DEAUTH and backup remain in place. Original upstream README, GIFs and CI/flasher workflows do not replace DNSP documents or release signing.

Source: https://github.com/skizzophrenic/SquachWatch-CYD/releases/tag/v1.28.0

Wi-Fi connection now has a connection-only screen, returns to saved networks, and retains that mode across password entry. Normal startup joins saved networks for time and label recall without checking firmware. The system no longer offers automatic Update Check. Only the explicit firmware update action checks updates.

Location labels use a checksummed, bounded compact NVS record instead of a padded 2296-byte snapshot on every save. Existing snapshots load and migrate on successful save; failed writes retain the active label and mapping. Backup snapshots retain their existing format. The video showed failed persistence, but did not establish the physical NVS error code; on-device label save/recall still needs confirmation.

Card content remains v1.5, unchanged. No flash erase, PIN reset, or card format is required.
