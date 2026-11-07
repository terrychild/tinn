#ifndef LIB_CRYPTO_X25519_H
#define LIB_CRYPTO_X25519_H

#include "lib/types.h"

BigInt* x25519PublicKey(Memory* mem, const BigInt* private_key);
BigInt* x25519(Memory* mem, const BigInt* scaler, const BigInt* point);

#endif