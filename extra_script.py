# DNSP v0.1 uses the reviewed upstream baseline for OTA/mesh version comparison.
# The UI separately identifies the DNSP draft.
Import("env")
import os


def get_version():
    # A bench override: SQW_VERSION=9.9.9 makes a build claim a version, so a
    # squad update nudge from it counts as newer on a board built from the
    # same tree. Never set in a release build.
    forced = os.environ.get("SQW_VERSION", "").strip()
    if forced:
        return forced
    return "1.19.1"  # DNSP v0.1 baseline; do not inherit a parent directory's git tag


env.Append(BUILD_FLAGS=['-DFIRMWARE_VERSION=\\"%s\\"' % get_version()])

# The environment name, as SQW_ENV. A Bluetooth update is signed for exactly one
# build and the board checks the signature against its own name, so an image
# for a different board -- a different display driver, say -- is refused
# rather than installed as a white screen. See include/ota_ble.h.
env.Append(BUILD_FLAGS=['-DSQW_ENV=\\"%s\\"' % env["PIOENV"]])


# Partition-table validation must include storage used outside named partitions.
import runpy
from pathlib import Path
_project = Path(env.subst('$PROJECT_DIR'))
runpy.run_path(str(_project / 'tools/check_flash_layout.py'))['check'](_project)
