#ifndef LIB_MATHS_MPA_H
#define LIB_MATHS_MPA_H

#include "lib/types.h"

void mpaSetMpa(U64 dest[], const U64 source[], U64 limbs);
void mpaSetInt(U64 dest[], U64 i, U64 limbs);
void mpaSetHex(U64 dest[], const char* hex, U64 limbs);
#define mpaSet(dest, source, limbs) _Generic((source), \
		U64*: mpaSetMpa, \
        const U64*: mpaSetMpa, \
        char*: mpaSetHex, \
        const char*: mpaSetHex, \
        default: mpaSetInt  \
    )(dest, source, limbs)

Slice mpaToBESlice(Allocator* allocator, const U64 source[], U64 limbs);

U64 mpaBitLength(const U64 source[], U64 limbs);
bool mpaBit(const U64 source[], U64 bit, U64 limbs);
bool mpaIsZero(const U64 source[], U64 limbs);
int mpaCmp(const U64 a[], const U64 b[], U64 limbs);
int mpaCmpInt(const U64 a[], U64 b, U64 limbs);

U64 mpaAdd(U64 a[], const U64 b[], U64 limbs);
U64 mpaSub(U64 a[], const U64 b[], U64 limbs);

#endif