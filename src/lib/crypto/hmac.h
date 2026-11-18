#ifndef LIB_CRYPTO_HMAC_H
#define LIB_CRYPTO_HMAC_H

#include "lib/types.h"

typedef Slice (*Hash)(Memory* mem, Reader* reader);

Slice hmac(Memory* mem, Slice key, Reader* reader, Size block_len, Hash hash_func);

Slice hmacSha224Reader(Memory* mem, const Slice key, Reader* reader);
Slice hmacSha224Slice(Memory* mem, const Slice key, Slice slice);
#define hmacSha224(mem, key, source) _Generic(source, \
    Reader*: hmacSha224Reader, \
    const Slice: hmacSha224Slice, \
    Slice: hmacSha224Slice \
)(mem, key, source)

Slice hmacSha256Reader(Memory* mem, const Slice key, Reader* reader);
Slice hmacSha256Slice(Memory* mem, const Slice key, Slice slice);
#define hmacSha256(mem, key, source) _Generic(source, \
    Reader*: hmacSha256Reader, \
    const Slice: hmacSha256Slice, \
    Slice: hmacSha256Slice \
)(mem, key, source)

Slice hmacSha384Reader(Memory* mem, const Slice key, Reader* reader);
Slice hmacSha384Slice(Memory* mem, const Slice key, Slice slice);
#define hmacSha384(mem, key, source) _Generic(source, \
    Reader*: hmacSha384Reader, \
    const Slice: hmacSha384Slice, \
    Slice: hmacSha384Slice \
)(mem, key, source)

Slice hmacSha512Reader(Memory* mem, const Slice key, Reader* reader);
Slice hmacSha512Slice(Memory* mem, const Slice key, Slice slice);
#define hmacSha512(mem, key, source) _Generic(source, \
    Reader*: hmacSha512Reader, \
    const Slice: hmacSha512Slice, \
    Slice: hmacSha512Slice \
)(mem, key, source)

#endif