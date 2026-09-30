#!/usr/bin/env python3
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zlib

spec = importlib.util.spec_from_file_location('verify_backup', Path(__file__).resolve().parents[1] / 'tools/verify_backup.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class RecoveryChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.backup, self.kit = root/'backup', root/'kit'
        self.backup.mkdir(); self.kit.mkdir()
        image = b'\xe9' + b'\0' * (0x1e0000-1)
        (self.backup/'firmware.bin').write_bytes(image)
        table = b'partition fixture'.ljust(3072, b'\xff')
        self.layout = root/'device-layout.bin'
        self.layout.write_bytes(table.ljust(4096, b'\xff'))
        body = b'DNSP_PUBLIC_V1=1\n'
        (self.backup/'preferences.txt').write_bytes(body + b'crc32=' + str(zlib.crc32(body)).encode() + b'\n')
        self.manifest = {'firmware':'DNSquachWatch v0.10.3-draft','build':'cyd-fast','app_bytes':str(len(image)), 'source_address':'0x1f0000','sha256':hashlib.sha256(image).hexdigest(),'app_only':'true','partition_sector_sha256':module.sha(self.layout)}
        self.save_marker()
        entries=[]
        for name, offset in [('bootloader.bin',0x1000),('partitions.bin',0x8000),('boot_app0.bin',0xe000),('firmware.bin',0x10000)]:
            (self.kit/name).write_bytes(table if name=='partitions.bin' else b'release test fixture')
            entries.append({'file':name,'offset':hex(offset),'bytes':(self.kit/name).stat().st_size,'sha256':module.sha(self.kit/name)})
        (self.kit/'manifest.json').write_text(json.dumps({'target':'cyd-fast','files':entries}))
    def save_marker(self):
        (self.backup/'COMPLETE.txt').write_text('DNSP_BACKUP_V1\n'+''.join(f'{k}={v}\n' for k,v in self.manifest.items()))
    def check(self): return module.check(self.backup,self.kit,self.layout)
    def test_valid(self): self.assertEqual(self.check(),self.backup/'firmware.bin')
    def test_v11_requires_log_snapshot(self):
        self.manifest['firmware']='DNSquachWatch v1.1.2';self.save_marker()
        with self.assertRaises(ValueError): self.check()
        self.manifest['current_log']='current-log.csv';self.manifest['current_log_rows']='0';self.save_marker()
        (self.backup/'current-log.csv').write_text('row,type,mac,rssi\n')
        self.assertEqual(self.check(),self.backup/'firmware.bin')
    def test_missing_marker(self):
        (self.backup/'COMPLETE.txt').unlink()
        with self.assertRaises(ValueError): self.check()
    def test_corrupt_image(self):
        with (self.backup/'firmware.bin').open('r+b') as f: f.seek(1024);f.write(b'bad')
        with self.assertRaises(ValueError): self.check()
    def test_wrong_board(self):
        self.manifest['build']='awok';self.save_marker()
        with self.assertRaises(ValueError): self.check()
    def test_wrong_device_layout(self):
        self.layout.write_bytes(b'\0'*4096)
        with self.assertRaises(ValueError): self.check()
    def test_corrupt_preferences(self):
        (self.backup/'preferences.txt').write_bytes(b'DNSP_PUBLIC_V1=1\ncrc32=0\n')
        with self.assertRaises(ValueError): self.check()
    def test_corrupt_release(self):
        (self.kit/'bootloader.bin').write_bytes(b'wrong')
        with self.assertRaises(ValueError): self.check()
    def test_duplicate_manifest(self):
        with (self.backup/'COMPLETE.txt').open('a') as f:f.write('build=cyd-fast\n')
        with self.assertRaises(ValueError): self.check()

if __name__=='__main__':unittest.main()
