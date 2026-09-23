# DNSquachWatch v0.5 — Breakout validation

Local draft, no push or flash. Settings → Breakout. Both orientations supported; three lives and 32 bricks, tap to launch, drag to steer. Games are session-only, including score; no SD dependency or extra full-screen buffer. New resets the round. Background detection remains enabled.

## Measured CYD ST7789 build

- Actual firmware image: 1,825,008 bytes, an increase of 3,456 bytes (3.38 KiB) over v0.4.
- Actual image headroom: 141,072 bytes (137.8 KiB).
- Linker application flash: 1,818,713/1,966,080 bytes.
- Static RAM: 101,984 bytes, 48 bytes more than v0.4.
- Display remains experimental 80 MHz; flash 40 MHz. Two OTA slots and all storage offsets are unchanged; generated partition binaries match v0.4 byte-for-byte.

## Passed checks

- 31 native unit suites, including Breakout collisions, scoring, ready/paused/win/loss transitions, paddle bounds and 100,000 deterministic physics steps.
- Three focused AddressSanitizer/UndefinedBehaviorSanitizer suites: Breakout, detection and security.
- Three complete setup/loop UI runs: landscape, portrait and reboot path.
- Two additional complete UI runs under ASan/UBSan: landscape and portrait, with screenshots reviewed.
- Six flash-layout checks.
- Four firmware builds: cyd, cyd-fast, cyd-ili9341, awok.
- Binary hashes and archive integrity checked during packaging.

UI checks enter the game via the real settings menu, launch and drag through calibrated touch, hold the steering finger during an ordinary detection, queue a second alert, and verify return to the exact paused ball/score/lives state. Resume requires user action. New, Back, lock and screen-timeout pause paths are exercised. Watched-device interrupt handling follows the same pause/return path in firmware, with a release guard; physical radio/watch delivery has not been tested.

The initial UI run exposed an existing menu-capacity assumption: repeated section headings can outnumber the six group types. The extra row triggered a stack-buffer overflow. Buffer capacity now covers a heading per row plus both dynamic tracking rows. Sanitized full-loop tests passed after the fix. The timeout test explicitly enables power saver before testing, then restores the previous settings.

## Limits

No physical board testing. Frame rate, minimum free heap/largest block, stack peaks, 80 MHz electrical stability and radio drop rates during gameplay still need device measurement. Static RAM figures do not include stack peaks. The game caps catch-up time after a stall, deliberately slowing play rather than simulating a long unseen interval. It is not a guarantee of real-time performance.

No audio, saved high scores, new language translations, new remote-control integrations or SD pack loader were added. Core labels for this minigame are English. Existing language/game/content roadmap items retain their prior status.

Reproduce with `make -C test`, `sh test/run_dnsp_ui.sh`, `python3 test/flash_layout_test.py` and the four PlatformIO environments. For the complete sanitized UI, build sim with `OBJ_DIR=build-asan LIVE_BIN=squachsim-asan EXTRA_CXXFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' live`; run with DNSP_UI_TEST=1 and a fresh SQUACHSIM_NVS directory. Add SQUACHSIM_ROTATE=1 and DNSP_UI_PORTRAIT=1 for portrait.
