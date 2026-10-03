#!/usr/bin/env python3
import hashlib,json,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
 d=Path(d);key=d/'key.pem';pub=d/'pub.pem';image=d/'image.bin';out=d/'manifest.json';payload=d/'payload'
 subprocess.run(['openssl','ecparam','-name','prime256v1','-genkey','-noout','-out',str(key)],check=True,capture_output=True)
 subprocess.run(['openssl','ec','-in',str(key),'-pubout','-out',str(pub)],check=True,capture_output=True)
 image.write_bytes(b'\xe9firmware-fixture')
 subprocess.run(['python3',str(root/'tools/sign_content_manifest.py'),'--key',str(key),'--image',str(image),'--target','cyd-fast','--version','1.5.3','--output',str(out)],check=True)
 body=out.read_bytes();m=json.loads(body);assert m['required_content']=='v1.5'
 assert m['signed_image_sha256']==hashlib.sha256(b'SQWOTA1\ncyd-fast\n'+image.read_bytes()).hexdigest()
 def verify(data):
  payload.write_bytes(data)
  return subprocess.run(['openssl','dgst','-sha256','-verify',str(pub),'-signature',str(out.with_suffix('.sig')),str(payload)],capture_output=True).returncode==0
 assert verify(b'DNSP_CONTENT1\ncyd-fast\n'+body)
 assert not verify(b'DNSP_CONTENT1\ncyd-ili9341-fast\n'+body)
 assert not verify(b'DNSP_CONTENT1\ncyd-fast\n'+body.replace(b'v1.5',b'v1.6'))
 assert not verify(b'DNSP_CONTENT1\ncyd-fast\n'+body.replace(m['signed_image_sha256'].encode(),b'0'*64))
print('Signed OTA contract: valid, tampered requirement/digest, wrong board PASS')
