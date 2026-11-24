#ifndef LIB_CRYPTO_HKDF_H
#define LIB_CRYPTO_HKDF_H

#include "lib/types.h"
#include "lib/crypto/hash.h"

Slice hkdfExtractReader(Memory* mem, HashAlgorithm hash, Slice salt, Reader* ikm);
Slice hkdfExtractSlice(Memory* mem, HashAlgorithm hash, Slice salt, Slice ikm);
#define hkdfExtract(mem, hash, salt, ikm) _Generic(ikm, \
    Reader*: hkdfExtractReader, \
    const Slice: hkdfExtractSlice, \
    Slice: hkdfExtractSlice \
)(mem, hash, salt, ikm)


#endif
