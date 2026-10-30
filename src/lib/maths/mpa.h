#ifndef LIB_MATHS_MPA_H
#define LIB_MATHS_MPA_H

#include "lib/types.h"

U64 mpaSetInt(U64 dest[], U64 limbs, U64 i);
U64 mpaSetHex(U64 dest[], U64 limbs, const char* hex);

Slice mpaToBESlice(Allocator* allocator, const U64 source[], U64 limbs);

U64 mpaBitLength(const U64 source[], U64 limbs);
bool mpaBit(const U64 source[], U64 limbs, U64 bit);
bool mpaIsZero(const U64 source[], U64 limbs);
int mpaCmp(const U64 a[], const U64 b[], U64 limbs);
int mpaCmpInt(const U64 a[], U64 b, U64 limbs);

U64 mpaAdd(U64 a[], const U64 b[], U64 limbs);
U64 mpaSub(U64 a[], const U64 b[], U64 limbs);

#endif