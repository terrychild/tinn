#ifndef LIB_CRYPTO_SHA_H
#define LIB_CRYPTO_SHA_H

#include "lib/types.h"

Slice sha_1(Memory* mem, Slice source);

Slice sha_2_224(Memory* mem, Slice source);
Slice sha_2_256(Memory* mem, Slice source);
Slice sha_2_384(Memory* mem, Slice source);
Slice sha_2_512(Memory* mem, Slice source);

#endif