Import("env")
from pathlib import Path
# The linker excludes image headers/alignment. Reject an actual oversized image
# even if PlatformIO's ELF-only check passes; both OTA slots stay unchanged.
def check_complete_image(source, target, env):
    image = Path(env.subst('$BUILD_DIR')) / 'firmware.bin'
    limit = int(env.BoardConfig().get('upload.maximum_size'))
    if image.stat().st_size > limit:
        raise RuntimeError('Complete firmware image is %d bytes; app slot is %d bytes' % (image.stat().st_size, limit))
env.AddPostAction('$BUILD_DIR/${PROGNAME}.bin', check_complete_image)

