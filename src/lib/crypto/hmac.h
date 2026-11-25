#ifndef LIB_CRYPTO_HMAC_H
#define LIB_CRYPTO_HMAC_H

#include "lib/types.h"
#include "lib/crypto/hash.h"

Slice hmac(Memory* mem, HashAlgorithm hash, Slice key, Slice message);

#endif