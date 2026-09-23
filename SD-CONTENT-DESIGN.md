# Shared games and remote content design

This is the approved design phase, not a claim that new games or universal-remote drivers are implemented. Existing features and all current language glyphs remain built in and work without microSD. v0.4 makes no new SD dependency.

## One firmware, optional richer content

Use a small common set of compiled engines and screen templates. Content packs on `/dnsp/content/` provide artwork, levels, dialogue, tutorials, palettes, device labels and layouts. The firmware retains touch input, rendering, rules, protocol drivers, authentication, command validation and scheduling. Packs cannot install native code, arbitrary scripts or arbitrary network operations.

Initial engines to design for: grid puzzles, timed one-touch minigames and turn-based rooms. A future fishing game can share timers, touch buttons, animation and saved progress with other games. A dungeon can read one room at a time. Remotes share navigation, media and light-control templates; layouts select predefined semantic commands, not executable URLs or scripts.

Remote control work remains at the research checkpoint in the separate CYD Remote Research design. Roku's documented-use restriction and Bluetooth interoperability questions remain unresolved. SD content does not remove those constraints.

## Proposed pack format, version 1

An offline builder converts editable JSON/text/paletted artwork to a bounded binary pack. The reader must validate before publishing a new pack index.

Header: magic `DNSPPAK1`, format version, declared file length, entry count (at most 256), content generation, and checksum. Index entries: resource ID, resource kind, offset, stored length, decoded length, width/height where applicable, and per-entry checksum. Fixed-width integers are little-endian, decoded explicitly rather than by unaligned casts. Reject duplicate IDs, unknown required versions, invalid dimensions, overlapping ranges, integer overflow, truncated files and decompression exceeding declared limits. CRC detects damage, not authenticity; any future trust/signature requirement is separate.

Kinds: bounded UTF-8 text; palette; indexed bitmap; tile map; game level; remote layout. Proposed limits: 8 MiB per pack, 4 KiB text entries, 32×32 tile maps, at most 24 remote controls per paginated layout. No absolute/parent paths or nested file references; resources refer to IDs within the same pack. The installer stages a new file, validates it, then switches generations while retaining the previous known-good version. FAT renames are not assumed power-loss atomic; boot recovery must select only a complete validated generation.

Bitmap palettes support at least the requested eight colors, with 16/256-color variants where useful. Tile data and optional compressed images decode into a bounded strip/tile cache. Effects and live drawing remain in the shared renderer, not large frame-by-frame videos.

## Resource budgets and ownership

Proposed initial total content working-memory budget: 8 KiB, to be verified on hardware. Use a small reusable sector/stream buffer plus a bounded tile cache; do not allocate another full-screen framebuffer. Limit reads per main-loop turn and yield to radio observations, alert presentation and pending log writes. Store offsets rather than whole resource copies in RAM. Larger assets increase SD use, not unlimited heap use.

The shared storage service owns card access. Radio callbacks never read/write packs. Card removal, busy state and I/O failures return explicit results. Safe shutdown closes content readers before sync/unmount. Firmware/card writes take priority over cosmetic asset loads. Cache recently displayed tiles, redraw only dirty regions and throttle animated content; establish budgets with measurements rather than claim guaranteed frame rates.

## Fallback behavior

Core menus, alerts, supported device controls and fonts stay in firmware initially. Missing artwork gets a built-in icon. Missing/invalid tutorial content gets concise built-in help. Optional game packs show "Content pack unavailable" and return cleanly; there is no blank-screen crash. Existing language selection remains usable without the card.

Moving existing essential fonts entirely to SD would change offline behavior and is not part of this release. Further font/table compression can preserve offline behavior. Optional extended fonts can later be loaded through the same resource interface.

## Shared remote controls

A layout references an existing saved-device ID, a template and semantic commands such as NavigateLeft, PlayPause, SetBrightness or SelectPreset. Drivers decide isSupported and validate each parameter. Unknown/unsupported commands remain disabled. Host addresses, tokens, Wi-Fi passwords and pairing material are stored separately; packs never supply or export them. Discovery cannot silently pair or send commands. Only the active user-selected remote obtains radio ownership.

## Save data

Keep user progress separate from read-only packs, keyed by stable game and schema IDs. Batch saves after a round/checkpoint rather than every frame. Use bounded alternating records with generation and checksum; recover the latest complete record after interruption. Keep radio evidence logs distinct from game state. Missing microSD should not reset or overwrite existing internal settings.

## Acceptance tests before implementation ships

Test malformed index counts/offsets, path traversal, bad checksums, oversized text/images, decompression bombs, missing card, card removal during reads, shutdown while a reader is active, partial installations and game saves. Check English/Hebrew/CJK fallbacks and both display orientations. Verify alert interruption/resume, radio queue drops, lowest heap/largest free block, stack peaks and frame latency during actual SD activity. Unsupported remote actions must send no network/Bluetooth command.

## Implementation order

1. Add and test a bounded read-only pack reader plus offline builder and validation fixtures, once the first game/remote controls are selected.
2. Integrate one optional artwork/help pack with built-in fallback; measure flash delta including decoder overhead and runtime cache cost.
3. Reuse the reader for levels and layouts; add one small game or approved driver at a time.
4. Publish pack/firmware compatibility manifests and a safe recovery procedure.

No expected flash saving is booked until bytes have actually moved out of the firmware and all fallbacks have been counted. Text/assets on SD cannot make compiled game engines or radio stacks disappear from flash.
