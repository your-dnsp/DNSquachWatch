"""Fail-closed receive guard for pinned NimBLE-Arduino 2.5.1 CYD builds.

Applied to a generated build copy, never to the installed dependency. The hook
runs before receive-side device construction, vector insertion or payload update.
It is a memory-pressure mitigation, not a promise of allocation success.
"""
def patch(source):
    if 'dnsp_ble_receive_room' in source:
        raise RuntimeError('NimBLE source already adapted')
    event_marker = '''# if CONFIG_BT_NIMBLE_EXT_ADV
            const auto& disc        = event->ext_disc;'''
    new_marker = '''                advertisedDevice = new NimBLEAdvertisedDevice(event, event_type);'''
    if source.count(event_marker) != 1 or source.count(new_marker) != 1:
        raise RuntimeError('NimBLE receive adapter needs review')
    source=source.replace(event_marker,'''            if (!dnsp_ble_receive_room()) return 0;

'''+event_marker)
    source=source.replace(new_marker,'''                // Callbacks-only mode still retains active-scan reply waiters.
                // Bound that temporary list; known records can still be updated.
                if (pScan->m_maxResults == 0 && pScan->m_scanResults.m_deviceVec.size() >= 32) {
                    dnsp_ble_receive_drop();
                    return 0;
                }
'''+new_marker)
    return 'extern "C" bool dnsp_ble_receive_room();\nextern "C" void dnsp_ble_receive_drop();\n'+source
