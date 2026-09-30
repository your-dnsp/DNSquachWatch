# Upstream v1.25.0 integration

Source: <https://github.com/skizzophrenic/SquachWatch-CYD/releases/tag/v1.25.0>

DNSquachWatch previously used the SquachWatch v1.20.0 foundation. This release selectively merges applicable original-project work through v1.25.0 while treating established DNSP decisions and menu organization as authoritative where the projects differ.

The v1.25.0 merge corrects printed Bluetooth MAC byte order with compatibility for older history, adds seven status-light brightness levels while preserving saved output levels, and makes Settings headings non-folding labels. The T-Watch-only GPS hardware, spare-flash wardrive store, and web-flasher controls were not copied onto the CYD. The hardware-independent NMEA parser is included for future external-GPS work; see `GPS-WIGLE-READINESS.md`.

Included upstream behavior:

- adaptive Wi-Fi channel dwell and the Wi-Fi driver restart timing margin;
- CYD Bluetooth-update removal for reduced flash and heap use;
- spam-flood detection with one alert per active flood;
- revised outfits and the LOCKED ON watch-list alert;
- additional time zones;
- Flock-You detection clues and upstream Flock signature additions;
- Bluetooth and Wi-Fi Remote ID fixes;
- ignored-device status and safer ownership of Bluetooth name data.

DNSP retains its expanded and independently tested ALPR, Axon, Meta, FPV, Remote ID, evidence/confidence, research/export, security, recovery, game, accessibility, menu, and microSD behavior. An upstream match never removes a stronger DNSP feature solely to make the source identical.

The v1.1.2 release builds and distributes only `cyd-fast`: the classic 2.8-inch ST7789 CYD with an initial 80 MHz display clock.
