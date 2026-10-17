"""Keep the pinned TZ parser on its reviewed bounded ASCII/integer formats.

Only a build-local copy of lib_a-tzset_r.o is adapted; libc and the toolchain
installation are not modified. All DST rules and timezone calculations remain
newlib's own code. A changed SDK object requires reviewing its formats again.
"""
from pathlib import Path
import hashlib,shutil,subprocess
# Pinned 8.4.0+2021r2-patch5 target objects from the official macOS and
# Linux x86_64 packages. Their allocated sections and code/relocations are
# identical; only non-runtime metadata differs. Unknown objects still fail closed.
EXPECTED=frozenset({
    'd11274047743a0ef61e1e0f8285bc5926d033ac9ed7c8c3cd31237fcf94490e5',
    'd1c1e32a5990152661be6ed93cf3ba7e45267aead9b1b4b42b73ccb991d161ea',
})
def adapt(source,dest,ar,objcopy):
    dest.mkdir(parents=True,exist_ok=True)
    subprocess.run([str(ar),'x',str(source),'lib_a-tzset_r.o'],cwd=dest,check=True)
    obj=dest/'lib_a-tzset_r.o'
    actual=hashlib.sha256(obj.read_bytes()).hexdigest()
    if actual not in EXPECTED:
        raise RuntimeError('Unreviewed timezone object: cannot apply bounded scanner: '+actual)
    subprocess.run([str(objcopy),'--redefine-sym','siscanf=dnspIntegerScanC',str(obj)],check=True)
    archive=dest/'libc.a';shutil.copyfile(source,archive)
    subprocess.run([str(ar),'r',str(archive),str(obj)],check=True)
    return archive
