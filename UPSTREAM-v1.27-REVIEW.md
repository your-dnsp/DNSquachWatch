# Upstream integration for DNSP v1.3

Compared SquachWatch v1.25.0 to tags v1.26.0 and v1.27.0 (v1.27.0 commit 3d2ab4f). Applied compatible shared source changes with a three-way merge; resolved conflicting menus and application states in favor of DNSP. Detection.cpp and detection.h have no upstream changes in this tag range; existing DNSP research signatures and per-source DEAUTH logic remain unchanged.

Included: privacy screen helpers across shared UI, charge/runtime tools adapted to CYD, Legend aura, wizard outfit/matching mesh slot, Terminal secret, previous/next time-zone selection, and password-keyboard redraw compatibility. DNSP's existing two-band keyboard fix remains intact.

Excluded from this CYD: the LoRa stack and watch-specific battery/GPS/radio-duty additions. This board has no compatible LoRa transceiver or watch PMU/GPS. The original project's rearranged menus were deliberately not imported. No BLE address-order or detection changes were made.

Charge Mode is guarded behind normal startup, so Duress Pixel Tide, Safe Mode and Safe Shutdown continue using their own loops. Failed radio ownership prevents entry. Active backup, readable refresh, research/capture or update work must finish first. Privacy Mode affects supported radio screens only; stored records, console output and user-opened raw files are not made anonymous.

Upstream releases: https://github.com/skizzophrenic/SquachWatch-CYD/releases/tag/v1.26.0 and https://github.com/skizzophrenic/SquachWatch-CYD/releases/tag/v1.27.0.

Charge Mode honors the existing PIN sleep/idle lock policy while the normal loop is paused; wake returns to the locked UI when needed. Manual BOOT screen-off overrides Auto Brightness and follows the existing wake-on-alert choice, including DNSP rule alerts.
