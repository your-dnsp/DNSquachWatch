"""Record actual image size and largest flash symbols; not a heap profiler."""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--nm',required=True);p.add_argument('--build',required=True);p.add_argument('--output',required=True);a=p.parse_args()
b=Path(a.build)
lines=subprocess.check_output([a.nm,'--print-size','--size-sort','--radix=d','-C',str(b/'firmware.elf')],text=True).splitlines()
symbols=[]
for line in lines:
    parts=line.split(None,3)
    if len(parts)!=4:continue
    try:addr=int(parts[0]);size=int(parts[1])
    except ValueError:continue
    # ESP32 mapped DROM and IROM; avoid claiming BSS uses image flash.
    if 0x3f400000<=addr<0x3f800000 or 0x400d0000<=addr<0x40400000:
        symbols.append(dict(bytes=size,kind=parts[2],name=parts[3]))
symbols.sort(key=lambda x:x['bytes'],reverse=True)
result={'image_bytes':(b/'firmware.bin').stat().st_size,'largest_mapped_flash_symbols':symbols[:60],
        'format_scan_symbols':[s for s in symbols if any(x in s['name'] for x in ('scanf','printf','strtod','dtoa'))],
        'notes':'Symbol sizes are not guaranteed independently recoverable savings; image includes headers/padding/IRAM data. RAM peaks require hardware measurements.'}
Path(a.output).write_text(json.dumps(result,indent=2)+'\n')
print('Image bytes:',result['image_bytes'])
