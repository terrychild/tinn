#ifndef LIB_CRYPTO_HMAC_H
#define LIB_CRYPTO_HMAC_H

#include "lib/types.h"
#include "lib/crypto/hash.h"

Slice hmacReader(Memory* mem, HashAlgorithm hash, Slice key, Reader* reader);
Slice hmacSlice(Memory* mem, HashAlgorithm hash, Slice key, Slice slice);
#define hmac(mem, hash, key, source) _Generic(source, \
    Reader*: hmacReader, \
    const Slice: hmacSlice, \
    Slice: hmacSlice \
)(mem, hash, key, source)

#endif