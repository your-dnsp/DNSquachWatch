# DNSquachWatch release-candidate version stamped throughout the image.
Import("env")
import os


def get_version():
    # A bench override: SQW_VERSION=9.9.9 makes a build claim a version, so a
    # squad update nudge from it counts as newer on a board built from the
    # same tree. Never set in a release build.
    forced = os.environ.get("SQW_VERSION", "").strip()
    if forced:
        return forced
    return "1.1.2"  # do not inherit a parent directory's git tag


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

# TFT_eSPI normally compiles its write clock into every transaction. Generate
# a narrow adapter without editing the installed library or changing touch/read
# clocks. Only these four pinned 2.8-inch CYD targets use it; DMA is not used here.
if env['PIOENV'] in ('cyd', 'cyd-fast', 'cyd-ili9341', 'cyd-ili9341-fast'):
    def runtime_display(env, node):
        source = Path(node.srcnode().get_abspath())
        header = source.parent / 'TFT_eSPI.h'
        if '#define TFT_ESPI_VERSION "2.5.43"' not in header.read_text():
            raise RuntimeError('Runtime display speed requires TFT_eSPI 2.5.43')
        transform = runpy.run_path(str(_project/'tools/runtime_display.py'))['patch']
        result = transform(source.read_text())
        destination = Path(env.subst('$BUILD_DIR'))/'dnsp-runtime'/'TFT_eSPI.cpp'
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.exists() or destination.read_text() != result:
            destination.write_text(result)
        return env.File(str(destination))
    env.AddBuildMiddleware(runtime_display, '*TFT_eSPI.cpp')

# Receive-side checks must precede NimBLE's allocations, not merely guard
# our callback after the device object/payload has already been allocated.
if env['PIOENV'] in ('cyd', 'cyd-fast', 'cyd-ili9341', 'cyd-ili9341-fast'):
    def runtime_ble(env, node):
        source = Path(node.srcnode().get_abspath())
        properties = source.parent.parent / 'library.properties'
        if 'version=2.5.1' not in properties.read_text().splitlines():
            raise RuntimeError('BLE receive guard requires NimBLE-Arduino 2.5.1')
        transform = runpy.run_path(str(_project/'tools/runtime_ble.py'))['patch']
        result = transform(source.read_text())
        destination = Path(env.subst('$BUILD_DIR'))/'dnsp-runtime'/'NimBLEScan.cpp'
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.exists() or destination.read_text() != result:
            destination.write_text(result)
        return env.File(str(destination))
    env.AddBuildMiddleware(runtime_ble, '*NimBLEScan.cpp')
