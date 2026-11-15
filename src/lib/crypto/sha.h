#ifndef LIB_CRYPTO_SHA_H
#define LIB_CRYPTO_SHA_H

#include "lib/types.h"
#include "lib/macros.h"

Slice sha1_Slices(Memory* mem, Slice* source, U64 source_count);
Slice sha1_Slice(Memory* mem, Slice source);
#define sha1(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    Slice: sha1_Slice, \
    Slice*: sha1_Slices \
)(mem, __VA_ARGS__)

Slice sha224_Slices(Memory* mem, Slice* source, U64 source_count);
Slice sha224_Slice(Memory* mem, Slice source);
#define sha224(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    Slice: sha224_Slice, \
    Slice*: sha224_Slices \
)(mem, __VA_ARGS__)

Slice sha256_Slices(Memory* mem, Slice* source, U64 source_count);
Slice sha256_Slice(Memory* mem, Slice source);
#define sha256(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    Slice: sha256_Slice, \
    Slice*: sha256_Slices \
)(mem, __VA_ARGS__)

Slice sha384_Slices(Memory* mem, Slice* source, U64 source_count);
Slice sha384_Slice(Memory* mem, Slice source);
#define sha384(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    Slice: sha384_Slice, \
    Slice*: sha384_Slices \
)(mem, __VA_ARGS__)

Slice sha512_Slices(Memory* mem, Slice* source, U64 source_count);
Slice sha512_Slice(Memory* mem, Slice source);
#define sha512(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    Slice: sha512_Slice, \
    Slice*: sha512_Slices \
)(mem, __VA_ARGS__)


#endif