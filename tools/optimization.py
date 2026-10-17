Import('env')
# The pinned platform disables LTO at link time by default. Compile and link
# must agree; preserve the prebuilt ESP-IDF libraries and root app_main.
env['LINKFLAGS']=[f for f in env['LINKFLAGS'] if f != '-fno-lto']
env.Append(LINKFLAGS=['-flto','-fuse-linker-plugin','-Wl,-u,app_main'])
# Archive indexing must understand LTO objects too.
env.Replace(AR='xtensa-esp32-elf-gcc-ar', RANLIB='xtensa-esp32-elf-gcc-ranlib')
# Large drawing paths otherwise duplicate small wrappers under LTO. Shared
# calls reduce flash size; SPI transfer remains the dominant display cost.

# This firmware is an HTTPS client, never a TLS server. Rejecting the unused
# role removes server handshake code retained by the pinned generic dispatcher.
env.Append(LINKFLAGS=['-Wl,--wrap=mbedtls_ssl_handshake_server_step'])

# Use the integer scanner for the pinned SDK's three-integer version query.
# Patch only a build-local, hash-checked archive; installed SDK is unchanged.
from pathlib import Path
import sys
sys.path.insert(0, env.subst('$PROJECT_DIR') + '/tools')
from runtime_coex import adapt
framework = Path(env.PioPlatform().get_package_dir('framework-arduinoespressif32'))
toolchain = Path(env.PioPlatform().get_package_dir('toolchain-xtensa-esp32')) / 'bin'
local_sdk = Path(env.subst('$BUILD_DIR')) / 'dnsp-sdk'
adapt(framework / 'tools/sdk/esp32/lib/libbt.a', local_sdk,
      toolchain / 'xtensa-esp32-elf-ar', toolchain / 'xtensa-esp32-elf-objcopy')
env.Prepend(LIBPATH=[str(local_sdk)])

# TZ's integer scanner is otherwise the sole remaining general scanner.
# Preserve the pinned timezone code and adapt only its reviewed call formats.
from runtime_timezone_scan import adapt as adapt_timezone
adapt_timezone(toolchain.parent / 'xtensa-esp32-elf/lib/no-rtti/libc.a', local_sdk,
               toolchain / 'xtensa-esp32-elf-ar', toolchain / 'xtensa-esp32-elf-objcopy')
