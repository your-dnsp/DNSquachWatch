"""Verify every decoded Remington photo pixel against the prior packed image hash."""
from pathlib import Path
import re, hashlib
root=Path(__file__).resolve().parents[1]
s=(root/'include/remmy_photo_data.inc').read_text()
def array(name):
 body=s.split(name,1)[1].split('{',1)[1].split('}',1)[0]
 return [int(x,0) for x in re.findall(r'0x[0-9a-fA-F]+|\b\d+\b',body)]
offsets=array('REMMY_PHOTO_ROWS');packed=array('REMMY_PHOTO_RLE');raw=bytearray()
assert len(offsets)==193 and offsets[-1]==len(packed)
for y in range(192):
 row=[]
 for v in packed[offsets[y]:offsets[y+1]]:row.extend([v&15]*((v>>4)+1))
 assert len(row)==164
 raw.extend((row[i]<<4)|row[i+1] for i in range(0,164,2))
assert hashlib.sha256(raw).hexdigest()==(root/'test/remmy_photo.sha256').read_text().strip()
print('Remington photo: all 31,488 pixels are identical; packed bytes',len(packed),'row index bytes',len(offsets)*2)
