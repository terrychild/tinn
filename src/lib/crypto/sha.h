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

#endif