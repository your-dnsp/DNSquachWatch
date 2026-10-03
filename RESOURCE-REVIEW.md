# v0.4 measured size audit

The 64 KiB-per-slot partition expansion was **not applied**. My prior assessment that the 128 KiB gap was unused was wrong: `src/blackbox.cpp` uses raw flash at 0x3D0000–0x3F0000 for persistent detection and boot records. Its overlap protection would disable history if this space became an application slot. Standard partition validation alone did not detect that feature regression.

The new build guard validates both named partitions and this raw reservation. Six regression checks cover the valid layout, both proposed expansions, overlap, alignment and flash overflow. Actual generated partition binaries are byte-identical between v0.3 and v0.4. Settings, history, coredump and both OTA slots remain in their original locations. No partition migration is included or required.

## Measured results: CYD ST7789 at 80 MHz

| Measurement | v0.3 | v0.4 | Change |
|---|---:|---:|---:|
| Linker-reported application flash | 1,820,509 | 1,815,249 | 5,260 fewer bytes |
| Actual firmware.bin | 1,826,800 | 1,821,552 | 5,248 fewer bytes |
| Static RAM | 101,936 | 101,936 | +0 bytes |
| OTA slot capacity | 1,966,080 | 1,966,080 | unchanged |
| Actual image headroom | 139,280 | 144,528 | 5,248 more bytes |

Actual image length includes headers/alignment and is the more conservative capacity measure. The previous approximately 142 KiB headroom statement used linker figures rather than full image length. Neither figure measures runtime free heap or stack peaks.

## Changes retained

All 646 bundled glyphs now encode a 16-bit row-change mask plus only rows that differ from the preceding row. Normal and Japanese variants remain independently addressable. Bitmap data shrank from 31,424 to 25,966 bytes (5,458 bytes). Index size stays 5,168 bytes. A bounded decoder uses a 32-byte local bitmap buffer, no persistent heap or second framebuffer. Its overhead explains why net firmware savings differ from raw bitmap savings. Every variant is checked against hashes computed from original source bytes, and truncated input is rejected.

No screen, effect, language, radio feature, protocol, authentication check or persistent record capacity was removed. Runtime source differences are the glyph decoder and v0.4 labels. Existing 80 MHz display and 40 MHz flash settings remain.

## Experiments and findings

- Replacing the application's two sscanf calls did not eliminate the scanner: Arduino IPv6Address.cpp still references it. The custom-parser experiment was reverted; it increased code without accomplishing the intended library removal.
- Link-time optimization was attempted in an isolated copy. The pinned toolchain failed with LTO plugin/link errors; no LTO flags enter the release. A toolchain migration would require its own compatibility work.
- The build already uses -Os, function/data sections and linker garbage collection. These are not new savings to claim.
- Large individual mapped symbols include mascot drawing, snowfall/starfield/backdrops and several formatting functions. Shared code and data can be investigated, but these symbols implement existing behavior and cannot simply be deleted. Their symbol sizes are not guaranteed independently recoverable savings.
- `tools/size_audit.py` generates image-size reports and lists the largest mapped flash symbols using the toolchain nm utility. Generated audit snapshots are not included in the repository.

## SD-based content

See SD-CONTENT-DESIGN.md for common engines/templates, bounded versioned content packs, caches, fallback behavior, radio/storage ownership and validation gates. This is the requested design, not a shipped pack loader or new games/remotes. Keeping existing functions available without a card means current essential glyphs and controls remain compiled in. Optional future artwork, dialogue and levels can live on SD; executable code does not automatically move there.

## Remaining constraints

This maintenance draft recovers roughly 5.1 KiB, not the promised 64 KiB. That promise depended on an incorrect storage assumption. More room requires further measured optimization, optional SD content, or an explicit future storage/hardware tradeoff. No feature-loss or partition change has been silently substituted.
