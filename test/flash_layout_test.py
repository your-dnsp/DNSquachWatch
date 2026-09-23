"""Regression: unnamed flash is not necessarily unused flash."""
from pathlib import Path
import importlib.util
import tempfile
import shutil
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('layout',ROOT/'tools/check_flash_layout.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Layout(unittest.TestCase):
    def changed(self,old,new):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d);(p/'src').mkdir()
            shutil.copy2(ROOT/'src/blackbox.cpp',p/'src/blackbox.cpp')
            s=(ROOT/'partitions_ota.csv').read_text()
            self.assertIn(old,s)
            (p/'partitions_ota.csv').write_text(s.replace(old,new))
            with self.assertRaises(ValueError):m.check(p)
    def test_current(self):self.assertEqual(m.check(ROOT),1966080)
    def test_proposed_reclaim_rejected(self):
        self.changed('0x1F0000, 0x1E0000','0x1F0000, 0x200000')
    def test_evenly_enlarged_slots_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d);(p/'src').mkdir();shutil.copy2(ROOT/'src/blackbox.cpp',p/'src/blackbox.cpp')
            s=(ROOT/'partitions_ota.csv').read_text().replace('0x10000,  0x1E0000','0x10000,  0x1F0000').replace('0x1F0000, 0x1E0000','0x200000, 0x1F0000')
            (p/'partitions_ota.csv').write_text(s)
            with self.assertRaisesRegex(ValueError,'BlackBox'):m.check(p)
    def test_overlapping_apps(self):self.changed('0x1F0000, 0x1E0000','0x1E0000, 0x1E0000')
    def test_unaligned_app(self):self.changed('0x1F0000, 0x1E0000','0x1F1000, 0x1E0000')
    def test_beyond_flash(self):self.changed('0x3F0000, 0x10000','0x3F0000, 0x20000')
if __name__=='__main__':unittest.main()
