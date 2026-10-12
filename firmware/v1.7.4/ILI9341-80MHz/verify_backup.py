#!/usr/bin/env python3
"""Read-only DNSP app-backup/release-kit checks. Never opens a serial port or flashes."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import zlib


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            h.update(chunk)
    return h.hexdigest()


def check(backup, kit, layout):
    def require(condition, message):
        if not condition:
            raise ValueError(message)
    marker = backup / 'COMPLETE.txt'
    require(marker.is_file() and marker.stat().st_size <= 4096, 'Missing/oversized completion manifest')
    lines = marker.read_text(encoding='ascii').splitlines()
    require(lines and lines[0] in {'DNSP_BACKUP_V1', 'DNSP_BACKUP_V2'}, 'Unsupported backup format')
    backup_format = lines[0]
    meta = {}
    for line in lines[1:]:
        if '=' in line:
            key, value = line.split('=', 1)
            require(key not in meta, 'Duplicate manifest field')
            meta[key] = value
    require(meta.get('app_only') == 'true', 'Not an application-only backup')
    require(meta.get('firmware') in {'DNSquachWatch v0.10.2-draft', 'DNSquachWatch v0.10.3-draft', 'DNSquachWatch v0.10.4-draft', 'DNSquachWatch v1.0.0-rc1', 'DNSquachWatch v1.1.2'}, 'Use the matching backup-version recovery tool')
    require(meta.get('app_bytes') == str(0x1e0000), 'Unexpected application-slot size')
    require(meta.get('source_address') in ('0x10000', '0x1f0000'), 'Unexpected source slot')
    app = backup / 'firmware.bin'
    require(app.is_file() and app.stat().st_size == 0x1e0000, 'Application size mismatch')
    require(sha(app) == meta.get('sha256'), 'Application SHA-256 mismatch')
    with app.open('rb') as f:
        require(f.read(1) == b'\xe9', 'Not an ESP image')
    prefs = backup / 'preferences.txt'
    require(prefs.is_file() and prefs.stat().st_size < 1024, 'Missing/oversized public preferences')
    text = prefs.read_bytes()
    body, separator, ending = text.rpartition(b'crc32=')
    require(separator and ending.endswith(b'\n') and ending[:-1].isdigit(), 'Malformed preferences checksum')
    require(zlib.crc32(body) == int(ending), 'Preferences checksum mismatch')
    if meta.get('firmware') == 'DNSquachWatch v1.1.2':
        require(meta.get('current_log') == 'current-log.csv', 'Missing current LOG snapshot declaration')
        require(meta.get('current_log_rows', '').isdigit() and int(meta['current_log_rows']) <= 64,
                'Invalid current LOG row count')
        log = backup / 'current-log.csv'
        require(log.is_file() and 0 < log.stat().st_size < 65536, 'Missing/oversized current LOG snapshot')
        with log.open('rb') as stream:
            require(stream.readline().startswith(b'row,type,mac,'), 'Malformed current LOG header')
    if backup_format == 'DNSP_BACKUP_V2':
        require(meta.get('operational_state') == 'operational-state.bin', 'Missing operational-state declaration')
        require(meta.get('readable_history') == 'complete', 'Missing readable-history declaration')
        state = backup / 'operational-state.bin'
        require(state.is_file() and 64 < state.stat().st_size < 8192, 'Missing/oversized operational state')
        for name in ('ALL-ALERTS.txt', 'SCAN-HISTORY.txt', 'SYSTEM-HISTORY.txt', 'EXPORT-SUMMARY.txt'):
            path = backup / name
            require(path.is_file() and path.stat().st_size < 4 * 1024 * 1024, 'Missing/oversized ' + name)
    manifest_path = kit / 'manifest.json'
    require(manifest_path.stat().st_size < 16384, 'Oversized release manifest')
    release = json.loads(manifest_path.read_text())
    require(release.get('target') == meta.get('build'), 'Board/build mismatch')
    expected = {'bootloader.bin': 0x1000, 'partitions.bin': 0x8000, 'boot_app0.bin': 0xe000, 'firmware.bin': 0x10000}
    entries = release.get('files', [])
    require(isinstance(entries, list) and len(entries) == 4, 'Unexpected release contents')
    found = set()
    for item in entries:
        name = item['file']
        require(name in expected and name not in found, 'Unexpected/duplicate filename')
        found.add(name)
        require(int(item['offset'], 16) == expected[name], 'Unexpected release offset')
        path = kit / name
        require(path.stat().st_size == item['bytes'] and sha(path) == item['sha256'], 'Release file hash/size mismatch: ' + name)
    table = (kit / 'partitions.bin').read_bytes()
    require(len(table) <= 4096, 'Oversized partition table')
    expected_layout = hashlib.sha256(table.ljust(4096, b'\xff')).hexdigest()
    require(expected_layout == meta.get('partition_sector_sha256'), 'Backup/release layout mismatch')
    require(layout.is_file() and layout.stat().st_size == 4096, 'Provide a 4096-byte current device partition-sector read')
    require(sha(layout) == expected_layout, 'Current device layout differs; stop for manual recovery review')
    return app


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backup', type=Path, required=True)
    parser.add_argument('--kit', type=Path, required=True)
    parser.add_argument('--device-layout', type=Path, required=True)
    args = parser.parse_args()
    try:
        app = check(args.backup, args.kit, args.device_layout)
    except (ValueError, KeyError, OSError, UnicodeError, TypeError) as e:
        parser.exit(1, 'NOT VERIFIED: ' + str(e) + '\n')
    print('Integrity and layout checks passed. This is not publisher authentication or a hardware recovery test.')
    print('Before writing, validate the ESP image with esptool image_info and read the recovery guide (docs/user/RECOVERY.md in the repository, RECOVERY.md in a release kit).')
    parts = ['python3', '-m', 'esptool', '--chip', 'esp32', '--port', 'PORT', 'write_flash', '--flash_mode', 'dio', '--flash_freq', '40m', '--flash_size', '4MB']
    for offset, path in [('0x1000', args.kit/'bootloader.bin'), ('0x8000', args.kit/'partitions.bin'), ('0xe000', args.kit/'boot_app0.bin'), ('0x10000', app)]:
        parts.extend([offset, str(path)])
    print('Manual recovery command (NOT executed; replace PORT):\n' + shlex.join(parts))

if __name__ == '__main__':
    main()
