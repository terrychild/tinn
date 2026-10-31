#ifndef LIB_CRYPTO_CURVE25519_H
#define LIB_CRYPTO_CURVE25519_H

#include "lib/types.h"

BigInt* curve25519Key(Allocator* allocator, BigInt* k);

#endif