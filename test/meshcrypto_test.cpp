// The REAL cipher, on the host, against the golden vectors.
//
// WHY THIS EXISTS, AND WHY IT IS NOT IN THE DEFAULT `make`.
//
// meshmsg_test.cpp says the device's cipher "cannot be built on a desktop in
// any form worth trusting", and for mbedtls 2 -- what espressif32@6.5.0 ships
// to every Xtensa board -- that is right: no distribution carries it any more.
// The ESP32-C5 environment is the exception. It builds on Arduino core 3.3 /
// IDF 5.5, which carries mbedtls 3.6, and mbedtls 3.6 is a normal package on
// an ordinary Linux box. So for THAT path, and only that path, the real
// meshcrypto.cpp can be compiled and run here.
//
// That matters because the C5 port had to migrate this file across the
// mbedtls 2 -> 3 API break (see include/mbedtls_compat.h), and NOTHING in the
// default host suite compiles meshcrypto.cpp at all. `make` passing therefore
// says nothing whatsoever about whether that migration changed the
// cryptography. This is the check that does, without needing the board.
//
// It is a separate target (`make -C test crypto`) because it needs
// libmbedtls-dev, which CI does not have. A test that silently skipped itself
// when a header was missing would be worse than no test: it would report green
// for a check that never ran.
//
//   sudo apt-get install libmbedtls-dev     # or the distro equivalent
//   make -C test crypto
#include "meshcrypto.h"
#include "meshmsg_vectors.h"
#include "test_util.h"
#include <mbedtls/version.h>
#include <cstring>
#include <cstdio>

int main() {
    printf("mbedtls %s (the C5 ships 3.6.6; the API shimmed in "
           "include/mbedtls_compat.h is chosen on MBEDTLS_VERSION_MAJOR, "
           "so any 3.x exercises the same branch)\n\n", MBEDTLS_VERSION_STRING);

    suite("The device's own boot self-test, run here");
    {
        // Key stretching against the golden key, the canned frame sealed and
        // opened byte for byte, a tampered frame REJECTED, and a text part
        // packed and unpacked -- all against frames built by Python's
        // `cryptography`, which shares no code with mbedtls.
        ck("selfTest passes", MeshCrypto::selfTest());
    }

    suite("SHA-256");
    {
        // NIST's one-block "abc" digest. selfTest never reaches
        // MeshCrypto::sha256, so without this the sqw_sha256 shim is unproven.
        static const uint8_t WANT[32] = {
            0xBA,0x78,0x16,0xBF,0x8F,0x01,0xCF,0xEA,0x41,0x41,0x40,0xDE,0x5D,0xAE,0x22,0x23,
            0xB0,0x03,0x61,0xA3,0x96,0x17,0x7A,0x9C,0xB4,0x10,0xFF,0x61,0xF2,0x00,0x15,0xAD,
        };
        uint8_t got[32];
        MeshCrypto::sha256((const uint8_t*)"abc", 3, got);
        ck("\"abc\" matches the published digest", memcmp(got, WANT, 32) == 0);
    }

    suite("X25519, the invite's key exchange");
    {
        // The ONLY thing that exercises SQW_ECP_X and SQW_ECP_Z -- mbedtls 3
        // made mbedtls_ecp_point's X and Z private, and reaching the wrong
        // member would not fail to compile, it would agree with nothing.
        uint8_t privA[32], pubA[32], privB[32], pubB[32];
        ck("a keypair is produced", MeshCrypto::dhKeypair(privA, pubA));
        ck("and a second one",      MeshCrypto::dhKeypair(privB, pubB));

        bool distinct = memcmp(pubA, pubB, 32) != 0;
        ck("the two public keys differ", distinct);

        uint8_t sAB[32], sBA[32];
        ck("A computes a shared secret", MeshCrypto::dhShared(privA, pubB, sAB));
        ck("B computes one too",         MeshCrypto::dhShared(privB, pubA, sBA));
        ck("and they are the SAME secret", memcmp(sAB, sBA, 32) == 0);

        // A secret both sides agree on but that is a constant would pass the
        // line above. It must depend on the keys.
        uint8_t privC[32], pubC[32], sAC[32];
        MeshCrypto::dhKeypair(privC, pubC);
        MeshCrypto::dhShared(privA, pubC, sAC);
        ck("a different peer gives a different secret", memcmp(sAB, sAC, 32) != 0);

        // A low-order public key must be refused, not turned into an all-zero
        // secret that both sides would happily agree on.
        uint8_t zero[32] = { 0 }, out[32];
        ck("an all-zero peer key is refused", !MeshCrypto::dhShared(privA, zero, out));
    }

    suite("What both sides derive from the secret");
    {
        uint8_t privA[32], pubA[32], privB[32], pubB[32];
        MeshCrypto::dhKeypair(privA, pubA);
        MeshCrypto::dhKeypair(privB, pubB);
        uint8_t sAB[32], sBA[32];
        MeshCrypto::dhShared(privA, pubB, sAB);
        MeshCrypto::dhShared(privB, pubA, sBA);

        uint8_t kA[MeshMsg::KEY_LEN], kB[MeshMsg::KEY_LEN];
        MeshCrypto::dhSessionKey(sAB, kA);
        MeshCrypto::dhSessionKey(sBA, kB);
        ck("both sides derive the same session key", memcmp(kA, kB, sizeof kA) == 0);

        uint8_t other[32] = { 0 };
        other[0] = 1;
        uint8_t kOther[MeshMsg::KEY_LEN];
        MeshCrypto::dhSessionKey(other, kOther);
        ck("a different secret gives a different key",
              memcmp(kA, kOther, sizeof kA) != 0);

        // The four digits people read to each other. Order-free by design, so
        // the two screens agree about which is A and which is B.
        const uint16_t cAB = MeshCrypto::dhCode(pubA, pubB);
        const uint16_t cBA = MeshCrypto::dhCode(pubB, pubA);
        ck("the confirmation code is order-free", cAB == cBA);
        ck("and is four digits", cAB < 10000);
    }

    return report();
}
