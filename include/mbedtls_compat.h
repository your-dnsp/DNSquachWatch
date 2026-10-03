// SquachWatch-CYD -- one place where mbedtls 2 and mbedtls 3 are reconciled.
//
// WHY THIS EXISTS. Every Xtensa board builds on the pinned espressif32@6.5.0
// (Arduino core 2.0.14 / IDF 4.4), which carries mbedtls 2.x. The ESP32-C5
// cannot: it needs Arduino core 3.3+ (IDF 5.5), which carries mbedtls 3.6.
// So two mbedtls major versions are in the project at once, and the handful of
// API changes between them land in the two files that must not be got wrong --
// meshcrypto.cpp (SquachMesh message encryption) and ota_core.cpp (the
// signature check that decides whether a firmware image is ours).
//
// NONE OF THESE SUBSTITUTIONS CHANGE THE CRYPTOGRAPHY. Each is a rename or an
// access-control change upstream made, not a different algorithm, curve, hash
// or parameter. That claim is not taken on trust in either file:
//   - meshcrypto is pinned by MeshCrypto::selfTest(), which rebuilds the golden
//     frames in include/meshmsg_vectors.h with mbedtls at every boot and
//     demands the same bytes. Those frames were produced by Python's
//     `cryptography`, which shares no code with mbedtls.
//   - ota_core is pinned by the release .sig files: a signature made by
//     tools/sign_firmware.py must still verify. A broken port here does not
//     weaken the check, it fails it, and the board refuses the update.
//
// Keep this header free of anything that is not a version shim. It is the
// place someone will look to answer "did the C5 port touch the crypto", and
// the answer has to stay readable in one screen.
#pragma once

#include <mbedtls/version.h>

#if MBEDTLS_VERSION_MAJOR >= 3

// mbedtls 3 made struct internals private. MBEDTLS_PRIVATE is mbedtls's own
// sanctioned escape hatch and expands to the same member access, so these are
// the identical fields under a longer name.
//
// The alternative -- defining MBEDTLS_ALLOW_PRIVATE_ACCESS -- would open every
// struct in every translation unit that saw it, which is a much larger blast
// radius than the four members actually needed here.
  #define SQW_ECP_X(p)     (p).MBEDTLS_PRIVATE(X)
  #define SQW_ECP_Z(p)     (p).MBEDTLS_PRIVATE(Z)
  #define SQW_ECDSA_GRP(c) (c).MBEDTLS_PRIVATE(grp)
  #define SQW_ECDSA_Q(c)   (c).MBEDTLS_PRIVATE(Q)

// In mbedtls 2 the plain SHA-256 names returned void and the _ret suffix meant
// "the one that reports errors". mbedtls 3 deleted the void versions, so the
// plain names now ARE the error-returning ones and the _ret aliases are gone.
// Same function, same digest; only the spelling moved.
  #define sqw_sha256_starts mbedtls_sha256_starts
  #define sqw_sha256_update mbedtls_sha256_update
  #define sqw_sha256_finish mbedtls_sha256_finish
  #define sqw_sha256        mbedtls_sha256

#else

  #define SQW_ECP_X(p)     (p).X
  #define SQW_ECP_Z(p)     (p).Z
  #define SQW_ECDSA_GRP(c) (c).grp
  #define SQW_ECDSA_Q(c)   (c).Q

  #define sqw_sha256_starts mbedtls_sha256_starts_ret
  #define sqw_sha256_update mbedtls_sha256_update_ret
  #define sqw_sha256_finish mbedtls_sha256_finish_ret
  #define sqw_sha256        mbedtls_sha256_ret

#endif
