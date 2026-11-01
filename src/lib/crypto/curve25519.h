#ifndef LIB_CRYPTO_CURVE25519_H
#define LIB_CRYPTO_CURVE25519_H

#include "lib/types.h"

BigInt* curve25519Key_b(Memory* mem, const BigInt* k);
BigInt* curve25519Key_h(Memory* mem, const char* k);
BigInt* curve25519Key_i(Memory* mem, U64 k);

#define curve25519Key(mem, k) _Generic((k), \
		BigInt*: curve25519Key_b, \
        const BigInt*: curve25519Key_b, \
        char*: curve25519Key_h, \
        const char*: curve25519Key_h, \
        default: curve25519Key_i  \
    )(mem, k)

#endif