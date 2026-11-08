#ifndef LIB_CRYPTO_SHA_H
#define LIB_CRYPTO_SHA_H

#include "lib/types.h"

Slice sha1(Memory* mem, Slice source);

Slice sha224(Memory* mem, Slice source);
Slice sha256(Memory* mem, Slice source);
Slice sha384(Memory* mem, Slice source);
Slice sha512(Memory* mem, Slice source);

#endif