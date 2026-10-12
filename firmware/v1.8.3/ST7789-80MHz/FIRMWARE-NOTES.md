DNSquachWatch v1.8.3 — ST7789-80MHz, SquachWatch base v1.35.0

Retains v1.8.2 controller separation and software resistive touch.
Single-sector writes now require an explicit accepted response and completed
programming before success. Multi-sector writes wait after the stop token.
CMD13 consumes its second status byte even when callers only request R1.

Test Card now reports accepted bytes, whether reopening succeeded, stored
file size, bytes read, content match and cleanup in serial as [sd-test].
Failure messages distinguish wrong size, short read and content mismatch.
File flush has no return value: read-back is required to verify persistence.

TARGETED TEST
Flash normally with the same card and existing DNSP Content/v1.5.
Do not erase the ESP32 or format/wipe the card.
Run Test Card once and capture serial at 115200 baud. Send the [sd-test]
and any [sd-write] lines along with the screen result. This build has normal
card writes enabled. If Test Card passes, try backup and Safe Shutdown.

DNSP reports that Test Card, backup, Safe Shutdown and reset/remount passed
on the tested device/card; touch appeared normal. This is scoped hardware
confirmation, not a guarantee for every card or a completed endurance test.
