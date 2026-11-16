#ifndef LIB_CRYPTO_SHA_H
#define LIB_CRYPTO_SHA_H

#include "lib/types.h"
#include "lib/macros.h"

Slice sha1_Slices(Memory* mem, const Slice* source, Size source_count);
Slice sha1_Slice(Memory* mem, const Slice source);
#define sha1(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: sha1_Slice, \
    Slice: sha1_Slice, \
    const Slice*: sha1_Slices, \
    Slice*: sha1_Slices \
)(mem, __VA_ARGS__)

Slice sha224_Slices(Memory* mem, const Slice* source, Size source_count);
Slice sha224_Slice(Memory* mem, const Slice source);
#define sha224(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: sha224_Slice, \
    Slice: sha224_Slice, \
    const Slice*: sha224_Slices, \
    Slice*: sha224_Slices \
)(mem, __VA_ARGS__)

Slice sha256_Slices(Memory* mem, const Slice* source, Size source_count);
Slice sha256_Slice(Memory* mem, const Slice source);
#define sha256(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: sha256_Slice, \
    Slice: sha256_Slice, \
    const Slice*: sha256_Slices, \
    Slice*: sha256_Slices \
)(mem, __VA_ARGS__)

Slice sha384_Slices(Memory* mem, const Slice* source, Size source_count);
Slice sha384_Slice(Memory* mem, const Slice source);
#define sha384(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: sha384_Slice, \
    Slice: sha384_Slice, \
    const Slice*: sha384_Slices, \
    Slice*: sha384_Slices \
)(mem, __VA_ARGS__)

Slice sha512_Slices(Memory* mem, const Slice* source, Size source_count);
Slice sha512_Slice(Memory* mem, const Slice source);
#define sha512(mem, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: sha512_Slice, \
    Slice: sha512_Slice, \
    const Slice*: sha512_Slices, \
    Slice*: sha512_Slices \
)(mem, __VA_ARGS__)

Slice hmacSha224_Slices(Memory* mem, const Slice key, const Slice* source, Size source_count);
Slice hmacSha224_Slice(Memory* mem, const Slice key, const Slice source);
#define hmacSha224(mem, key, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: hmacSha224_Slice, \
    Slice: hmacSha224_Slice, \
    const Slice*: hmacSha224_Slices, \
    Slice*: hmacSha224_Slices \
)(mem, key, __VA_ARGS__)

Slice hmacSha256_Slices(Memory* mem, const Slice key, const Slice* source, Size source_count);
Slice hmacSha256_Slice(Memory* mem, const Slice key, const Slice source);
#define hmacSha256(mem, key, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: hmacSha256_Slice, \
    Slice: hmacSha256_Slice, \
    const Slice*: hmacSha256_Slices, \
    Slice*: hmacSha256_Slices \
)(mem, key, __VA_ARGS__)

Slice hmacSha384_Slices(Memory* mem, const Slice key, const Slice* source, Size source_count);
Slice hmacSha384_Slice(Memory* mem, const Slice key, const Slice source);
#define hmacSha384(mem, key, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: hmacSha384_Slice, \
    Slice: hmacSha384_Slice, \
    const Slice*: hmacSha384_Slices, \
    Slice*: hmacSha384_Slices \
)(mem, key, __VA_ARGS__)

Slice hmacSha512_Slices(Memory* mem, const Slice key, const Slice* source, Size source_count);
Slice hmacSha512_Slice(Memory* mem, const Slice key, const Slice source);
#define hmacSha512(mem, key, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const Slice: hmacSha512_Slice, \
    Slice: hmacSha512_Slice, \
    const Slice*: hmacSha512_Slices, \
    Slice*: hmacSha512_Slices \
)(mem, key, __VA_ARGS__)

#endif