"""Keep the pinned TZ parser on its reviewed bounded ASCII/integer formats.

Only a build-local copy of lib_a-tzset_r.o is adapted; libc and the toolchain
installation are not modified. All DST rules and timezone calculations remain
newlib's own code. A changed SDK object requires reviewing its formats again.
"""
from pathlib import Path
import hashlib,shutil,subprocess
EXPECTED='d11274047743a0ef61e1e0f8285bc5926d033ac9ed7c8c3cd31237fcf94490e5'
def adapt(source,dest,ar,objcopy):
    dest.mkdir(parents=True,exist_ok=True)
    subprocess.run([str(ar),'x',str(source),'lib_a-tzset_r.o'],cwd=dest,check=True)
    obj=dest/'lib_a-tzset_r.o'
    if hashlib.sha256(obj.read_bytes()).hexdigest()!=EXPECTED:
        raise RuntimeError('Unreviewed timezone object: cannot apply bounded scanner')
    subprocess.run([str(objcopy),'--redefine-sym','siscanf=dnspIntegerScanC',str(obj)],check=True)
    archive=dest/'libc.a';shutil.copyfile(source,archive)
    subprocess.run([str(ar),'r',str(archive),str(obj)],check=True)
    return archive
