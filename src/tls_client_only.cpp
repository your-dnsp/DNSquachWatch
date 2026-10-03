// DNSP only opens outbound TLS connections for public firmware files. The
// pinned mbedTLS library's generic dispatcher otherwise links its entire
// HTTPS server handshake despite no firmware feature running a TLS server.
// Refuse that unused role explicitly; client handshake and signature checks
// remain intact. This must be revisited if a TLS server is ever introduced.
#if defined(ARDUINO_ARCH_ESP32)
#include <mbedtls/ssl.h>
extern "C" int __wrap_mbedtls_ssl_handshake_server_step(mbedtls_ssl_context*) {
    return MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;
}
#endif
