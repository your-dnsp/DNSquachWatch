from pathlib import Path
import importlib.util
import struct
import unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('journal',root/'tools/inspect_duress.py')
j=importlib.util.module_from_spec(spec);spec.loader.exec_module(j)
def record(m,v):
    words=(m,v,v^0xffffffff)
    return struct.pack('<IIII',*words,j.checksum(words))
def sector(tail=b''):
    data=record(0x44555039,1)+tail
    return data+b'\xff'*(4096-len(data))
class Journal(unittest.TestCase):
    def test_length(self):
        with self.assertRaises(ValueError):j.inspect(b'')
    def test_empty(self):self.assertIn('No recognized',j.inspect(b'\xff'*4096))
    def test_pending(self):self.assertIn('PENDING',j.inspect(sector()))
    def test_torn(self):self.assertIn('PENDING',j.inspect(sector(record(0x54494439,0)[:9])))
    def test_errors(self):
        text=j.inspect(sector(record(0x54494439,15)))
        self.assertIn('COMPLETED',text)
        for name in ['NVS','BlackBox','Coredump','microSD']:self.assertIn(name,text)
    def test_success(self):self.assertIn('not proof of secure erasure',j.inspect(sector(record(0x54494439,0))))
if __name__=='__main__':unittest.main()
