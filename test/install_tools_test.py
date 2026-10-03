import importlib.util
from pathlib import Path
import tempfile
import hashlib
import json
from unittest.mock import patch
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from install_dnsp import install_card, entries, flash
from check_research_report import check
from research_issue_guard import screen

def rejects(fn):
    try:fn()
    except ValueError:return
    raise AssertionError('Operation should have been refused')
with tempfile.TemporaryDirectory() as temp:
    root=Path(temp);content=root/'microSD-content';source=content/'DNSP Content/v1.5';source.mkdir(parents=True)
    body=b'example card content';(source/'guide-000.txt').write_bytes(body)
    manifest={'files':{'DNSP Content/v1.5/guide-000.txt':hashlib.sha256(body).hexdigest()}}
    (content/'manifest.json').write_text(json.dumps(manifest));card=root/'card';card.mkdir();(card/'my-log.txt').write_bytes(b'keep')
    with patch('install_dnsp.os.path.ismount',return_value=True):
        install_card(content,card,True,False);assert not (card/'DNSP Content').exists()
        install_card(content,card,False,False);assert (card/'DNSP Content/v1.5/guide-000.txt').read_bytes()==body
        target=card/'DNSP Content/v1.5/guide-000.txt';target.write_bytes(b'changed');rejects(lambda:install_card(content,card,False,False))
        install_card(content,card,False,True);assert target.read_bytes()==body;assert (card/'my-log.txt').read_bytes()==b'keep'
        rejects(lambda:entries(content,{'files':{'../my-log.txt':'bad'}}))
        rejects(lambda:entries(content,{'files':{'my-log.txt':'bad'}}))
        outside=root/'outside';outside.mkdir();(card/'DNSP Content/v1.5').rename(card/'saved');(card/'DNSP Content/v1.5').symlink_to(outside,target_is_directory=True)
        rejects(lambda:install_card(content,card,False,True))
    with patch('install_dnsp.os.path.ismount',return_value=False):rejects(lambda:install_card(content,card,True,False))
    kit=root/'kit';kit.mkdir();(kit/'manifest.json').write_text(json.dumps({'display':'ST7789','target':'cyd-fast'}));lines=[]
    for name in ('bootloader.bin','partitions.bin','boot_app0.bin','firmware.bin'):
        p=kit/name;p.write_bytes(name.encode());lines.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+name)
    (kit/'SHA256SUMS').write_text('\n'.join(lines))
    with patch('install_dnsp.subprocess.run') as run,patch('install_dnsp.importlib.util.find_spec',return_value=True):
        flash(kit,'/dev/example',False);cmd=run.call_args.args[0];assert cmd.count('--port')==1 and cmd[-1]==str((kit/'firmware.bin').resolve());assert not run.call_args.kwargs.get('shell',False)
        flash(kit,'/dev/example',True);assert run.call_count==1
        (kit/'firmware.bin').write_bytes(b'corrupt');rejects(lambda:flash(kit,'/dev/example',False))
report={'schema':'dnsp-device-research-v1','status':'unverified','scope':'individual-device-observation','observed_mac':'AA:BB:CC:XX:XX:XX','identifiers_included':False,'advertised_name_bytes':''}
text=json.dumps(report);assert not check(text) and not screen(text)
report['observed_mac']='AA:BB:CC:11:22:33';assert check(json.dumps(report)) and screen(json.dumps(report))
assert screen(text+'\nhttps://github.com/user-attachments/files/1/private.txt')
report['observed_mac']='AA:BB:CC:XX:XX:XX';report['identifiers_included']=True;assert check(json.dumps(report))
assert screen('{"schema":"dnsp-device-research-v1"}')
report.update(schema='dnsp-device-research-v2',identifiers_included=False,simulated=True,simulation_subtag='SIMULATED',address_provenance='claimed transmitter')
assert not check(json.dumps(report)) and not screen(json.dumps(report))
del report['simulated']
assert check(json.dumps(report)) and screen(json.dumps(report))
report.update(simulated=True,simulation_subtag='')
assert check(json.dumps(report))
report.update(simulated=False,simulation_subtag='')
assert not check(json.dumps(report))
print('Installer + submission security checks PASS')
