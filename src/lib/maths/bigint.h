#ifndef LIB_MATHS_BIGINT_H
#define LIB_MATHS_BIGINT_H

#include "lib/types.h"

struct BigInt {
    Allocator* allocator;
	U64 size;
	U64 length;
	U64* limbs;
	struct BigInt* mod;
	bool negative;
};

BigInt* biNew(Allocator* allocator);
void biInit(BigInt* bi, Allocator* allocator, U64 size);
BigInt* biCopy(BigInt* dest, const BigInt* source);
BigInt* biClone(Allocator* allocator, const BigInt* source);

BigInt* biSetInt(BigInt* bi, I64 i);
BigInt* biFromInt(Allocator* allocator, I64 i);
BigInt* biSetHex(BigInt* bi, const char* hex);
BigInt* biFromHex(Allocator* allocator, const char* hex);
void biSetMod(BigInt* bi, BigInt* mod);

Slice biToBESlice(Allocator* allocator, const BigInt* bi);

bool biBit(const BigInt* bi, U64 bit);
bool biIsZero(const BigInt* bi);
int biCmpAbs(const BigInt* a, const BigInt* b);
int biCmp(const BigInt* a, const BigInt* b);
int biCmpInt(const BigInt* a, I64 b);

BigInt* biShiftLeft(BigInt* bi, U64 count);
BigInt* biShiftRight(BigInt* bi, U64 count);

BigInt* biAdd(BigInt* a, BigInt* b);
BigInt* biSub(BigInt* a, BigInt* b);
BigInt* biMul(BigInt* a, BigInt* b);
BigInt* biDiv(BigInt* a, BigInt* b, BigInt* r);
BigInt* biMod(BigInt* a, BigInt* b);
BigInt* biPow(BigInt* a, BigInt* b);

#endif