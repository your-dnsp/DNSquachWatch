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
