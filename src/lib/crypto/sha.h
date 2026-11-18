#ifndef LIB_CRYPTO_SHA_H
#define LIB_CRYPTO_SHA_H

#include "lib/types.h"

Slice sha_1_Reader(Memory* mem, Reader* reader);
Slice sha_1_Slice(Memory* mem, Slice slice);
#define sha_1(mem, source) _Generic(source, \
    Reader*: sha_1_Reader, \
    const Slice: sha_1_Slice, \
    Slice: sha_1_Slice \
)(mem, source)

Slice sha_2_224_Reader(Memory* mem, Reader* reader);
Slice sha_2_224_Slice(Memory* mem, Slice slice);
#define sha_2_224(mem, source) _Generic(source, \
    Reader*: sha_2_224_Reader, \
    const Slice: sha_2_224_Slice, \
    Slice: sha_2_224_Slice \
)(mem, source)

Slice sha_2_256_Reader(Memory* mem, Reader* reader);
Slice sha_2_256_Slice(Memory* mem, Slice slice);
#define sha_2_256(mem, source) _Generic(source, \
    Reader*: sha_2_256_Reader, \
    const Slice: sha_2_256_Slice, \
    Slice: sha_2_256_Slice \
)(mem, source)

Slice sha_2_384_Reader(Memory* mem, Reader* reader);
Slice sha_2_384_Slice(Memory* mem, Slice slice);
#define sha_2_384(mem, source) _Generic(source, \
    Reader*: sha_2_384_Reader, \
    const Slice: sha_2_384_Slice, \
    Slice: sha_2_384_Slice \
)(mem, source)

Slice sha_2_512_Reader(Memory* mem, Reader* reader);
Slice sha_2_512_Slice(Memory* mem, Slice slice);
#define sha_2_512(mem, source) _Generic(source, \
    Reader*: sha_2_512_Reader, \
    const Slice: sha_2_512_Slice, \
    Slice: sha_2_512_Slice \
)(mem, source)

#endif