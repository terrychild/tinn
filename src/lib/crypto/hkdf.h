#ifndef LIB_CRYPTO_HKDF_H
#define LIB_CRYPTO_HKDF_H

#include "lib/types.h"
#include "lib/crypto/hash.h"

Slice hkdfExtract(Memory* mem, HashAlgorithm hash, Slice salt, Slice ikm);
Slice hkdfExpand(Memory* mem, HashAlgorithm hash, Slice prk, Slice info, Size length);

#endif
