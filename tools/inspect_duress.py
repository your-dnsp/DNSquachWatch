"""Read-only decoder for a v0.9 duress journal captured with esptool."""
from pathlib import Path
import struct
import sys

ERRORS = {1: 'NVS erase/readback failed', 2: 'BlackBox erase/readback failed',
          4: 'Coredump erase/readback failed', 8: 'microSD unavailable or I/O/sync failure',
          16: 'microSD traversal/time bound reached', 32: 'Journal write/readback failed'}

def checksum(words):
    h = 2166136261
    for b in struct.pack('<III', *words):
        h = ((h ^ b) * 16777619) & 0xffffffff
    return h

def record(data, offset, magic):
    m, v, inv, crc = struct.unpack_from('<IIII', data, offset)
    return v if m == magic and inv == v ^ 0xffffffff and crc == checksum((m, v, inv)) else None

def inspect(data):
    if len(data) != 4096:
        raise ValueError('Expected exactly 4096 bytes from flash address 0x3FF000.')
    intent = record(data, 0, 0x44555039)
    if intent != 1:
        return 'FAULT / damaged intent' if data[:4] == struct.pack('<I', 0x44555039) else 'No recognized v0.9 intent'
    result = record(data, 16, 0x54494439)
    if result is None:
        return 'PENDING: wipe will be retried on the next compatible firmware boot.'
    lines = ['COMPLETED: persistent PIXEL TIDE selected.']
    lines += [text for bit, text in ERRORS.items() if result & bit]
    if result & ~63:
        lines += [f'Unknown failure bits: 0x{result & ~63:x}']
    if not result:
        lines += ['No recorded failures in the implemented checks. This is not proof of secure erasure.']
    return '\n'.join(lines)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('Usage: python tools/inspect_duress.py duress-journal.bin')
    try:
        print(inspect(Path(sys.argv[1]).read_bytes()))
    except (OSError, ValueError) as e:
        sys.exit(str(e))
