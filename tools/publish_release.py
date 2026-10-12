#!/usr/bin/env python3
"""Publish already-checked release assets. Never builds or signs firmware."""
import hashlib,json,os,re,urllib.request,urllib.error
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def release_assets(version):
 archives=[];lines=[];boards=[]
 for board,target in [('ST7789','cyd-fast'),('ILI9341','cyd-ili9341-fast')]:
  folder=ROOT/('firmware/v'+version)/(board+'-80MHz')
  if not folder.exists():continue
  name='DNSquachWatch-v'+version+'-'+board+'-80MHz.zip'
  manifest=json.loads((folder/'manifest.json').read_text());assert manifest['internal_version']==version and manifest['target']==target
  for entry in manifest['files']:
   p=folder/entry['file'];assert p.parent==folder and hashlib.sha256(p.read_bytes()).hexdigest()==entry['sha256']
  archive=folder/name;assert archive.is_file();archives.append(archive);boards.append(board)
  lines.append(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+name+'\n')
 if not archives:raise ValueError('No checked firmware kits for '+version)
 sums=ROOT/('firmware/v'+version)/'RELEASE-SHA256SUMS';sums.write_text(''.join(lines))
 return archives,sums,boards

def main():
 repo=os.environ['GITHUB_REPOSITORY'];commit=os.environ['GITHUB_SHA'];token=os.environ['GITHUB_TOKEN']
 if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+',repo) or not re.fullmatch(r'[0-9a-f]{40}',commit):raise ValueError('Invalid release target')
 version=re.search(r'^#define DNSP_RELEASE_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"$',(ROOT/'include/firmware_version.h').read_text(),re.M).group(1)
 archives,sums,boards=release_assets(version)
 title='DNSquachWatch v'+version+' — '+' / '.join(boards)+', 80MHz'
 base='https://api.github.com/repos/'+repo
 def api(path,method='GET',data=None,binary=False):
  url=path if path.startswith('https://uploads.github.com/') else base+path
  if path.startswith('https://') and not path.startswith('https://uploads.github.com/'):raise ValueError('Unexpected upload host')
  payload=data if binary else None if data is None else json.dumps(data).encode()
  req=urllib.request.Request(url,data=payload,method=method,headers={'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json','Content-Type':'application/octet-stream' if binary else 'application/json','X-GitHub-Api-Version':'2022-11-28'})
  with urllib.request.urlopen(req,timeout=120) as f:
   return {} if f.status==204 else json.load(f)
 try:release=api('/releases/tags/v'+version)
 except urllib.error.HTTPError as e:
  if e.code!=404:raise
  release=api('/releases','POST',dict(tag_name='v'+version,target_commitish=commit,name=title,body=(ROOT/('.github/release-notes/v'+version+'.md')).read_text(),draft=True,prerelease=False))
 existing=api('/releases/'+str(release['id'])+'/assets');upload=release['upload_url'].split('{')[0]
 for p in archives+[sums]:
  digest='sha256:'+hashlib.sha256(p.read_bytes()).hexdigest()
  old=next((a for a in existing if a['name']==p.name),None)
  if old and old.get('digest')==digest and old['size']==p.stat().st_size:continue
  if old:
   raise ValueError('Published asset differs: '+p.name)
  asset=api(upload+'?name='+p.name,'POST',p.read_bytes(),True);assert asset['size']==p.stat().st_size
  if asset.get('digest'):assert asset['digest']==digest
 api('/releases/'+str(release['id']),'PATCH',dict(draft=False,name=title,body=(ROOT/('.github/release-notes/v'+version+'.md')).read_text()))
 print('Published v'+version+' with checked '+' / '.join(boards)+' ZIPs and SHA256.')
if __name__=='__main__':main()
