#ifndef LIB_CRYPTO_SHA_H
#define LIB_CRYPTO_SHA_H

#include "lib/types.h"

Slice sha1Reader(Memory* mem, Reader* reader);
Slice sha1Slice(Memory* mem, Slice slice);
#define sha1(mem, source) _Generic(source, \
    Reader*: sha1Reader, \
    const Slice: sha1Slice, \
    Slice: sha1Slice \
)(mem, source)

Slice sha224Reader(Memory* mem, Reader* reader);
Slice sha224Slice(Memory* mem, Slice slice);
#define sha224(mem, source) _Generic(source, \
    Reader*: sha224Reader, \
    const Slice: sha224Slice, \
    Slice: sha224Slice \
)(mem, source)

Slice sha256Reader(Memory* mem, Reader* reader);
Slice sha256Slice(Memory* mem, Slice slice);
#define sha256(mem, source) _Generic(source, \
    Reader*: sha256Reader, \
    const Slice: sha256Slice, \
    Slice: sha256Slice \
)(mem, source)

Slice sha384Reader(Memory* mem, Reader* reader);
Slice sha384Slice(Memory* mem, Slice slice);
#define sha384(mem, source) _Generic(source, \
    Reader*: sha384Reader, \
    const Slice: sha384Slice, \
    Slice: sha384Slice \
)(mem, source)

Slice sha512Reader(Memory* mem, Reader* reader);
Slice sha512Slice(Memory* mem, Slice slice);
#define sha512(mem, source) _Generic(source, \
    Reader*: sha512Reader, \
    const Slice: sha512Slice, \
    Slice: sha512Slice \
)(mem, source)


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