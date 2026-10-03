"""Validate partitions AND the legacy raw-flash BlackBox reservation."""
from pathlib import Path
import csv
import re

def number(s):
    s=s.strip(); mult=1
    if s[-1:].upper() in ('K','M'):
        mult=1024 if s[-1].upper()=='K' else 1024*1024;s=s[:-1]
    return int(s,0)*mult

def check(root):
    root=Path(root)
    code=(root/'src/blackbox.cpp').read_text()
    # This checker validates the four CYD targets, not the upstream watch.
    # Select the CYD branch before extracting the raw-flash reservation.
    code=re.sub(r'#if defined\(SQW_S3\).*?#else(.*?)#endif', r'\1', code, flags=re.S)
    base=int(re.search(r'BASE\s*=\s*(0x[0-9A-Fa-f]+)',code)[1],16)
    sector=int(re.search(r'SECTOR\s*=\s*(\d+)',code)[1])
    sectors=int(re.search(r'SECTORS\s*=\s*(\d+)',code)[1])
    regions=[('partition table',0x8000,0x9000),('BlackBox',base,base+sector*sectors)]
    names=set();apps={};data={}
    for row in csv.reader((root/'config/partitions/cyd.csv').read_text().splitlines()):
        if not row or row[0].strip().startswith('#'):continue
        name,kind,sub,offset,size=(v.strip() for v in row[:5])
        if name in names:raise ValueError('Duplicate partition '+name)
        names.add(name);start=number(offset);length=number(size);end=start+length
        if length<=0 or start<0x9000 or end>0x400000:raise ValueError('Outside 4 MiB flash: '+name)
        if start%0x1000 or length%0x1000:raise ValueError('Sector alignment: '+name)
        if kind=='data':data[name]=(sub,start,length)
        if kind=='app':
            if start%0x10000:raise ValueError('Application alignment: '+name)
            apps[sub]=(start,length)
        for other,lo,hi in regions:
            if start<hi and end>lo:raise ValueError(name+' overlaps '+other)
        regions.append((name,start,end))
    if not all(k in apps for k in ('ota_0','ota_1')):raise ValueError('Two OTA slots required')
    if apps['ota_0'][1]!=apps['ota_1'][1]:raise ValueError('OTA slots must have equal capacities')
    if data.get('duress')!=('0x40',0x3ff000,0x1000):raise ValueError('Dedicated duress journal required')
    if data.get('coredump')!=('coredump',0x3f0000,0xf000):raise ValueError('Coredump must stop before journal')
    return apps['ota_0'][1]

if __name__=='__main__':
    import sys
    size=check(Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1])
    print(f'Flash layout safe: two {size}-byte OTA slots; BlackBox reserved')
