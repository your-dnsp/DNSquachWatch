#!/usr/bin/env python3
"""Install verified card content and/or flash a DNSquachWatch release kit.
Never formats a card or downloads content. A full board erase is opt-in and confirmed.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def entries(root, manifest):
    result=[]
    for name, expected in manifest['files'].items():
        rel=PurePosixPath(name)
        if rel.is_absolute() or '..' in rel.parts or '\\' in name or rel.parts[:2]!=('DNSP Content','v1.5'):
            raise ValueError('Unsafe manifest path')
        path=root.joinpath(*rel.parts)
        if path.is_symlink() or not path.is_file() or root.resolve() not in path.resolve().parents:
            raise ValueError('Missing or unsafe content: '+name)
        if digest(path)!=expected:
            raise ValueError('Content checksum failed: '+name)
        result.append((rel,path,expected))
    return result


def install_card(root, target, dry, replace):
    target=target.resolve()
    if not target.is_dir() or target==Path(target.anchor):
        raise ValueError('Choose the mounted microSD volume, not a filesystem root')
    if not os.path.ismount(target):
        raise ValueError('The selected card path is not a mounted volume')
    manifest=json.loads((root/'manifest.json').read_text())
    files=entries(root,manifest)
    for rel,src,expected in files:
        dest=target.joinpath(*rel.parts)
        # Refuse symlink escapes on cards/filesystems supporting them.
        if target not in dest.resolve().parents:
            raise ValueError('Card destination escapes selected volume')
        if dest.exists() and (not dest.is_file() or dest.is_symlink()):
            raise ValueError('Unsafe existing destination: '+str(dest))
        if dest.exists() and digest(dest)!=expected and not replace:
            raise ValueError('Different existing content; review then use --replace-content: '+str(dest))
    if dry:
        print('Would verify/copy',len(files),'content files to',target)
        return
    needed=sum(src.stat().st_size for _,src,_ in files)
    if shutil.disk_usage(target).free<needed:
        raise ValueError('Not enough card space')
    for rel,src,expected in files:
        dest=target.joinpath(*rel.parts);dest.parent.mkdir(parents=True,exist_ok=True)
        if dest.exists() and digest(dest)==expected:continue
        pending=dest.with_name(dest.name+'.installing')
        if pending.is_symlink():raise ValueError('Unsafe pending destination')
        try:
            with src.open('rb') as a,pending.open('wb') as b:
                shutil.copyfileobj(a,b,64*1024);b.flush();os.fsync(b.fileno())
            if digest(pending)!=expected:raise ValueError('Card read-back failed')
            pending.replace(dest)
        finally:
            if pending.exists():pending.unlink()
    print('Card content verified. Eject the card safely before inserting it into the powered-off CYD.')


def flash(kit, port, dry, erase=False):
    manifest=json.loads((kit/'manifest.json').read_text())
    if (manifest.get('display'),manifest.get('target')) not in [('ST7789','cyd-fast'),('ILI9341','cyd-ili9341-fast')]:
        raise ValueError('This installer expects the ST7789 80 MHz release kit')
    # Use SHA256SUMS, which covers the complete kit, including firmware images.
    sums={}
    for line in (kit/'SHA256SUMS').read_text().splitlines():
        sha,name=line.split(maxsplit=1);sums[name.lstrip('*')]=sha
    images=[('0x1000','bootloader.bin'),('0x8000','partitions.bin'),('0xe000','boot_app0.bin'),('0x10000','firmware.bin')]
    for _,name in images:
        p=kit/name
        if p.is_symlink() or not p.is_file() or digest(p)!=sums.get(name):
            raise ValueError('Firmware checksum failed: '+name)
    cmd=[sys.executable,'-m','esptool','--chip','esp32','--port',port,'write_flash','--flash_mode','dio','--flash_freq','40m','--flash_size','4MB']
    for address,name in images:cmd.extend([address,str((kit/name).resolve())])
    print('Flashing',manifest['display'],'at display 80 MHz; flash bus 40 MHz.')
    if erase:
        print('FULL ERASE deletes PIN, duress / Pixel Tide state, Wi-Fi, settings, and onboard history. microSD is untouched; backups do not contain security secrets.')
        if not dry:
            try: answer=input('Erase the entire ESP32 before flashing? [y/N] ').strip().lower()
            except EOFError: answer=''
            if answer!='y':
                print('Erase declined. Nothing flashed.');return
        cmd.insert(cmd.index('--flash_mode'),'--erase-all')
    else:print('Saved settings and security state are retained.')
    if dry:
        import shlex
        print('Would run:',shlex.join(cmd));return
    if importlib.util.find_spec('esptool') is None:
        raise ValueError('Install esptool first: python3 -m pip install --user esptool')
    subprocess.run(cmd,check=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--kit',type=Path,default=Path(__file__).resolve().parent)
    p.add_argument('--card',type=Path,help='Mounted microSD volume, e.g. /Volumes/DNSP or /media/name/DNSP')
    p.add_argument('--port',help='Serial device; supplying this requests flashing')
    p.add_argument('--dry-run',action='store_true')
    p.add_argument('--erase',action='store_true',help='Full recovery erase; requires y confirmation. Removes PIN, duress state and onboard data')
    p.add_argument('--content',type=Path,help='Separate microSD-content folder; defaults to kit/microSD-content')
    p.add_argument('--replace-content',action='store_true',help='Replace only differing version-specific bundled content; never user logs')
    args=p.parse_args()
    if args.erase and not args.port:p.error('--erase requires --port')
    if not args.card and not args.port:p.error('Supply --card, --port, or both')
    kit=args.kit.resolve()
    try:
        if args.card:install_card(args.content or kit/'microSD-content',args.card,args.dry_run,args.replace_content)
        if args.port:flash(kit,args.port,args.dry_run,args.erase)
    except (OSError,ValueError,KeyError,subprocess.CalledProcessError) as e:
        print('Installation stopped:',e,file=sys.stderr);return 1
    return 0
if __name__=='__main__':sys.exit(main())
