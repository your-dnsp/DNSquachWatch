"""Validate versioned card fonts/translations against retained source fixtures."""
from pathlib import Path
import json,re,struct
r=Path(__file__).resolve().parents[1]
s=(r/'include/language_data.h').read_text()
rows=[json.loads('['+x+']') for x in re.findall(r'^\{(".*")\},?$',s,re.M)]
rows=[row for row in rows if len(row)==7]
font=re.findall(r'static const uint8_t glyphBits\[\]=\{(.*?)\};',s,re.S)[-1]
expected=bytes(int(x.strip(),0) for x in font.split(',') if x.strip())
c=r/'microSD-content/DNSP Content/v1.5'
assert (c/'glyphs.bin').read_bytes()==expected
blob=(c/'translations.bin').read_bytes();at=0;count=0
for row in rows:
 for value in row[1:]:
  n,h=struct.unpack_from('<HI',blob,at);data=blob[at+6:at+6+n];assert 0<n<512 and len(data)==n
  checksum=2166136261
  for b in data:checksum=((checksum^b)*16777619)&0xffffffff
  assert checksum==h and data.decode()==value
  at+=6+n;count+=1
assert at==len(blob)
manifest=json.loads((r/'microSD-content/manifest.json').read_text())
import hashlib
for path,digest in manifest['files'].items():assert hashlib.sha256((r/'microSD-content'/path).read_bytes()).hexdigest()==digest
print(f'Validated {count} translation records, {len(expected)} font bytes and every card-content hash.')
