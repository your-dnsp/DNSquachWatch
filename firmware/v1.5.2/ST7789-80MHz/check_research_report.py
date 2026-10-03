#!/usr/bin/env python3
"""Offline check before submitting a REDACTED device report. No uploads."""
import argparse
import json
from pathlib import Path
import re

FULL_MAC=re.compile(r'(?i)(?<![0-9a-f])(?:[0-9a-f]{2}[:-]){5}[0-9a-f]{2}(?![0-9a-f])')
MASKED_MAC=re.compile(r'(?i)^[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:XX:XX:XX$')

def check(text):
    errors=[]
    if FULL_MAC.search(text):errors.append('Contains a complete MAC address; submit only the REDACTED copy.')
    try:r=json.loads(text)
    except (ValueError,TypeError):return errors+['Report must be the exported JSON text.']
    if not isinstance(r,dict):return errors+['Report must be a JSON object.']
    if r.get('schema')!='dnsp-device-research-v1':errors.append('Unknown report schema.')
    if r.get('identifiers_included') is not False:errors.append('Identifiers must be excluded.')
    if not MASKED_MAC.fullmatch(str(r.get('observed_mac',''))):errors.append('MAC suffix must be XX:XX:XX.')
    if r.get('advertised_name_bytes')!='':errors.append('Advertised name must be omitted.')
    if r.get('status')!='unverified' or r.get('scope')!='individual-device-observation':errors.append('Report must remain an unverified individual observation.')
    return errors

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=Path);a=p.parse_args()
    if a.report.stat().st_size>32768:p.error('Not a small research report')
    errors=check(a.report.read_text())
    for e in errors:print(e)
    if not errors:print('Redaction checks passed. Also review free text for personal information; retain PRIVATE locally.')
    return bool(errors)
if __name__=='__main__':raise SystemExit(main())
