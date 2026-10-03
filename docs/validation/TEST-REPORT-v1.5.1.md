# v1.5.1 validation

DNSP confirmed backup completion on a physical ST7789-80MHz device, using the existing v1.5 content. The supplied serial output at 115200 baud showed continued operation without panic. Optional missing checkpoint journal files can produce Arduino filesystem diagnostics during discovery; these messages alone do not establish backup failure. A verified completion message and COMPLETE.txt identify a completed backup.

Native regression and UI simulation passed before the delivered image; full 1800-event tests cover linear bounded history reads, interruption/retry, no duplicates, write failures, CLR boundary, snapshot head and overwritten snapshots. Exact image: 1950096 bytes; partition limit1966080, remainder15984. Hardware confirmation does not establish all-card or all-unit reliability. Avata field validation and human translation review remain pending.

Linux CI's missing <algorithm> include in readable_storage_test.cpp has been corrected explicitly, including in the SD test shim. Workflow now runs installer/submission safety tests and card-content validation as well as native and ESP32 compilation. GitHub Release is gated on successful checks and publishes the checked hardware-tested image, not a CI-built replacement.
