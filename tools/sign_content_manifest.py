#!/usr/bin/env python3
"""Create a board-bound signed OTA content contract; never uploads it."""
import argparse, hashlib, json, subprocess, tempfile
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--image',type=Path,required=True)
    p.add_argument('--key',type=Path,required=True)
    p.add_argument('--target',choices=['cyd-fast','cyd-ili9341-fast'],required=True)
    p.add_argument('--version',required=True)
    p.add_argument('--required-content',default='v1.5')
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    digest=hashlib.sha256(b'SQWOTA1\n'+a.target.encode()+b'\n'+a.image.read_bytes()).hexdigest()
    body=(json.dumps(dict(version=a.version,required_content=a.required_content,signed_image_sha256=digest),separators=(',',':'))+'\n').encode()
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        payload=Path(tmp)/'contract';payload.write_bytes(b'DNSP_CONTENT1\n'+a.target.encode()+b'\n'+body)
        subprocess.run(['openssl','dgst','-sha256','-sign',str(a.key),'-out',str(a.output.with_suffix('.sig')),str(payload)],check=True)
    a.output.write_bytes(body)
if __name__=='__main__':main()
