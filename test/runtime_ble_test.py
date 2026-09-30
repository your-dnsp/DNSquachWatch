from pathlib import Path
import importlib.util
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('adapter',ROOT/'tools/runtime_ble.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class ReceiveGuard(unittest.TestCase):
 def setUp(self):self.raw=(ROOT/'.pio/libdeps/cyd-fast/NimBLE-Arduino/src/NimBLEScan.cpp').read_text()
 def test_before_allocations(self):
  s=m.patch(self.raw)
  guard=s.index('if (!dnsp_ble_receive_room())')
  self.assertLess(guard,s.index('new NimBLEAdvertisedDevice'))
  self.assertLess(guard,s.index('advertisedDevice->update(event'))
 def test_callbacks_only_bounded(self):
  s=m.patch(self.raw)
  self.assertIn('m_maxResults == 0 && pScan->m_scanResults.m_deviceVec.size() >= 32',s)
  self.assertLess(s.index('dnsp_ble_receive_drop();',s.index('int NimBLEScan::handleGapEvent')),s.index('new NimBLEAdvertisedDevice'))
 def test_reject_changed_library(self):
  with self.assertRaises(RuntimeError):m.patch(self.raw.replace('new NimBLEAdvertisedDevice(event, event_type)','new ChangedDevice(event)'))
 def test_reject_double_patch(self):
  with self.assertRaises(RuntimeError):m.patch(m.patch(self.raw))
if __name__=='__main__':unittest.main()
