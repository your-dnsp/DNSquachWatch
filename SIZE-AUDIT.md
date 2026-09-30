# DNSquachWatch v1.1.2 measured size audit

Primary comparison target: `cyd-fast`, classic 2.8-inch ST7789 CYD, 80 MHz initial display clock. The matching `cyd-ili9341-fast` release was subsequently measured at 1,912,313 linker bytes, 1,918,608 binary bytes, 47,472 bytes of raw slot headroom, and the same 120,128 bytes of static RAM.

## Final measurements

| Measurement | v1.1.1 | v1.1.2 | Change |
|---|---:|---:|---:|
| Linker application flash | 1,919,109 bytes | 1,912,213 bytes | **-6,896 bytes** |
| Linker-reported slot free | 46,971 bytes | 53,867 bytes | **+6,896 bytes** |
| Generated `firmware.bin` | 1,925,408 bytes | 1,918,512 bytes | **-6,896 bytes** |
| Binary headroom in the 1,966,080-byte slot | 40,672 bytes | 47,568 bytes | **+6,896 bytes** |
| Static RAM | 120,128 bytes | 120,128 bytes | unchanged |

The complete ST7789 binary now occupies **97.58%** of its application slot. Static RAM remains **36.7%**. This is a real improvement, but the slot is still close enough that another medium-sized feature should be preceded by another size pass.

## Changes retained

- The 10,568-byte two-display installation guide is stored as a 5,267-byte bounded LZ stream. A small decoder writes it directly to the backup file in 128-byte pieces. The guide remains built into the firmware, and its decoded bytes are tested against `DNSQUACHWATCH INSTALLATION.txt` exactly.
- The aquarium caustic texture is a box-filtered 32x32 tile instead of 64x64. Its flash data falls from 4,096 bytes to 1,024 bytes while remaining tileable and using no permanent RAM.
- No feature, language, theme, recovery partition, OTA slot, BlackBox region, or coredump region was removed.

## Optimization tested and rejected

The firmware's `sscanf` and `atof` call sites were replaced experimentally with small bounded parsers. The clean production build became **856 bytes larger**. The ESP32 framework already links the general conversion routines for other uses, so this firmware did not recover those library symbols and instead paid for the new parser code as well. The experiment was reverted rather than retaining an unmeasured assumption.

## Resource behavior

Guide decoding temporarily uses approximately 1.2 KiB of stack only at the end of a backup while writing the installation file. It does not allocate heap or reserve permanent RAM. The output is streamed and bounded, and malformed packed data stops the write instead of reading outside either buffer.

`SIZE-AUDIT-v1.1.2.json` contains the reproducible top-symbol snapshot from the final ELF. Symbol sizes are not guaranteed independently recoverable savings; runtime heap fragmentation, task-stack margin, display stability, and microSD latency still require physical testing.
