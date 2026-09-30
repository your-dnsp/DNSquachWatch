# Duress PIN and PIXEL TIDE — v1.1.2

This is an optional destructive feature. Read this before enabling it, and test only with expendable data. The software is built and tested on a host; physical power interruption, card failures and display behavior still need CYD testing. A screen PIN does not encrypt flash or microSD.

## Enable

1. Install the complete matching DNSquachWatch release kit for the supported 2.8-inch CYD display. An application-only install can retain an incompatible partition layout, so it is not sufficient for first installation.
2. In **Settings → Security**, enable PIN Lock. The Duress PIN row is hidden until PIN Lock is enabled.
3. Open Duress PIN, read the warning, choose Continue, verify your ordinary PIN, and enter a different duress PIN twice. It must have the same length as your ordinary PIN.
4. The stronger behavior must be explicitly enabled again after upgrading from earlier firmware. The old duress PIN is inactive; the ordinary PIN is unchanged.

Only entering this PIN at the **lock screen** activates it. Entering it during a settings verification does not wipe. The existing forgotten-PIN and ten-wrong-attempt options are separate legacy history/secret-clearing behaviors; they do not invoke PIXEL TIDE.

## What happens

The firmware verifies a persistent intent record before deleting anything. If it cannot establish that record, it stays locked and reports that it could not start. It closes active backup/storage handles and restarts into an isolated boot path before wiping, so normal log producers cannot recreate files during the wipe. A probationary OTA image must be confirmed before this restart to prevent rollback into an older application that cannot honor the journal.

The isolated path attempts and checks:

- All 20 KiB of NVS: preferences, PINs, credentials, calibration, progress and other stored settings. Nothing is copied back.
- All 128 KiB of BlackBox history.
- All 60 KiB of the current crash-dump partition.
- Root entries whose names begin with `dnsp-` or `squachwatch-`. Owned directories are traversed recursively, including DNSP firmware and preferences backups. Before removing each file, up to its first 512 bytes are overwritten and read back. Other root entries and files inside unrelated directories are left alone.
- Synchronization and unmounting of the card after the attempt.

Internal-flash erases are read back sector by sector. SD work is bounded to 2,048 traversal operations, eight directory levels and a 7.5-second total wipe-work budget. Slow/blocking hardware calls can exceed that budget. Missing, unsupported, failing or very large cards can leave data behind. Completion stores only error flags, not filenames or device observations.

A neutral **Loading…** screen appears during the operation, normally for about eight seconds on the isolated boot. The runtime transition also includes a restart. Completion then opens the original, fixed-memory **PIXEL TIDE** ripple toy. Touch the water to add ripples. A later boot goes straight to it: no normal scanning, mesh, logs, bookmarks, SD mounting, or update services start. It uses built-in touch ranges and the kit's initial display clock because settings were erased. There is no hidden exit gesture.

If power fails after intent commits but before completion commits, the next boot retries the wipe, then enters PIXEL TIDE. If completion was recorded with errors, later boots remain in PIXEL TIDE without remounting the card or retrying it. A missing card inserted later is therefore not automatically cleaned. An unreadable/recognizably damaged journal fails closed into the visual without claiming that erasure succeeded.

## Limits

This is **a partial quick wipe, not secure erasure, anti-forensic protection, or a whole-card format**. No raw-sector SD destruction was added. FAT deletion and header writes cannot reliably erase flash-controller remapped pages, other file contents, removed cards, computer backups, the inactive firmware image, or data an attacker already copied. A firmware reflash or physical readout can reveal the installed software and the journal; the visual is not guaranteed to deceive an examiner.

The persistent journal occupies the final 4 KiB of flash, outside NVS and the crash-dump region. Both application slots still hold 1,966,080 bytes. Downgrading to software that does not understand the journal can restore ordinary operation; this design does not prevent deliberate physical reflashing.

## Recover ordinary firmware / reset an unreadable display setting

Recovery is deliberately a computer-and-USB operation. It does not restore wiped data. First copy any files you want to retain from the card to your computer. Keep your earlier full-device backup if you may need it. If you want to inspect the wipe result before resetting, read the journal as described below.

Extract the complete matching release kit, follow `DNSQUACHWATCH INSTALLATION.txt`, and replace `PORT` with the actual serial port. The following command **erases all 4 MiB of internal flash**, including the decoy journal, remaining firmware, settings and history. It does not erase microSD.

```sh
python -m esptool --chip esp32 --port PORT erase_flash
python -m esptool --chip esp32 --port PORT write_flash \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

Use a normal 40 MHz kit if recovering from display instability. Complete first-time setup again. Ordinary upgrades use the non-erasing instructions in README; this full erase is specifically an intentional recovery/reset.

An SD firmware backup must use the release kit whose partition-table fingerprint matches its manifest. Do not bypass the recovery verifier or mix an old application backup with a different table. A full 4 MiB computer backup can restore its original complete layout, but also restores any settings, history, and journal present when it was captured.

## Inspect the result from a computer

The visual intentionally does not show wipe status. You can read the non-identifying journal over USB before recovery:

```sh
python -m esptool --chip esp32 --port PORT read_flash 0x3FF000 0x1000 duress-journal.bin
python tools/inspect_duress.py duress-journal.bin
```

The checker reports pending, completed, or damaged/missing records and any recorded failures. A completed record with no error bits means the implemented checks passed, not that physical secure erasure occurred. Successful completion on a desktop fake backend is not a hardware erase test.

The app-owned `/crash-reports/` and `/Device Health Export/` folders are included in recursive SD cleanup. The internal pending-report queue is inside NVS and covered by the existing NVS erase. Other unrelated card folders remain outside this bounded cleanup.
