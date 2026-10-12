"""Use newlib's integer scanner for the pinned SDK's integer-only version query.

Patch a build-local archive, never the installed SDK. The original SDK object
is hash-checked; an SDK change requires reviewing its formats again. This avoids
retaining the floating-point scanner solely for "%u.%u.%u" in libbt's coexistence
version query. Radio behavior and the result of that query are unchanged.
"""
from pathlib import Path
import hashlib, shutil, subprocess
EXPECTED = '837f21c5c31d25843a7bfdb15e0c73904a72089bb36b74667e15e16849fb2cd1'
def adapt(source, dest, ar, objcopy):
    dest.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(ar), 'x', str(source), 'bt.c.obj'], cwd=dest, check=True)
    obj = dest/'bt.c.obj'
    if hashlib.sha256(obj.read_bytes()).hexdigest() != EXPECTED:
        raise RuntimeError('Unreviewed Bluetooth SDK object: cannot apply integer version scanner')
    subprocess.run([str(objcopy), '--redefine-sym', 'sscanf=siscanf', str(obj)], check=True)
    archive=dest/'libbt.a';shutil.copyfile(source, archive)
    subprocess.run([str(ar), 'r', str(archive), str(obj)], check=True)
    return archive
