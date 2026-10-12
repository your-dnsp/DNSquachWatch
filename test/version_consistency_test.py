#!/usr/bin/env python3
"""Release labels must use the shared identity; stored-format names may be older."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
header=(root/'include/firmware_version.h').read_text()
version=re.search(r'^#define DNSP_RELEASE_VERSION "(\d+\.\d+\.\d+)"$',header,re.M).group(1)
for name in ['ui_boot.cpp','ui_sysprops.cpp','main.cpp','backup.cpp','research_submission.cpp','backup_maintenance.cpp']:
 text=(root/'src'/name).read_text()
 assert '#include "firmware_version.h"' in text,name
 assert not re.search(r'(?:DNSquachWatch|DNSP|Current DNSP firmware:) v\d+\.\d+\.\d+',text),name
assert 'include/firmware_version.h' in (root/'tools/build_version.py').read_text()
assert 'include/firmware_version.h' in (root/'tools/publish_release.py').read_text()
assert f'# DNSquachWatch v{version}' in (root/'README.md').read_text()
assert f'Current: v{version}' in (root/'firmware/README.md').read_text()
print('Shared version, on-device labels, report labels and release tools: PASS',version)

base=re.search(r'^#define DNSP_BASE_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"$',header,re.M).group(1)
for name in ['ui_boot.cpp','ui_sysprops.cpp','main.cpp','backup.cpp']:
 text=(root/'src'/name).read_text()
 assert 'DNSP_BASE_VERSION' in text,name
 assert not re.search(r'(?:base[:=]?|Based on SquachWatch v) ?[0-9]+\.[0-9]+\.[0-9]+',text),name
assert f'SquachWatch v{base}' in (root/'README.md').read_text()
print('Shared upstream base labels: PASS',base)
