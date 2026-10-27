#ifndef LIB_BIGINT_H
#define LIB_BIGINT_H

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
BigInt* biClone(Allocator* allocator, BigInt* source);

BigInt* biSetInt(BigInt* bi, I64 i);
BigInt* biFromInt(Allocator* allocator, I64 i);
BigInt* biSetHex(BigInt* bi, char* hex);
BigInt* biFromHex(Allocator* allocator, char* hex);
void biSetMod(BigInt* bi, BigInt* mod);

Slice biToBESlice(BigInt* bi);

bool biIsZero(BigInt* bi);
int biCmp(BigInt* a, BigInt* b);
int biCmpAbs(BigInt* a, BigInt* b);
int biCmpInt(BigInt* a, I64 b);

bool biBit(BigInt* bi, U64 bit);

BigInt* biShiftLeft(BigInt* dest, BigInt* source, U64 count);
BigInt* biShiftRight(BigInt* dest, BigInt* source, U64 count);

BigInt* biAdd(BigInt* dest, BigInt* a, BigInt* b);
BigInt* biSub(BigInt* dest, BigInt* a, BigInt* b);
BigInt* biMul(BigInt* dest, BigInt* a, BigInt* b);
BigInt* biDiv(BigInt* dest, BigInt* a, BigInt* b, BigInt* r);
BigInt* biMod(BigInt* r, BigInt* a, BigInt* b);
BigInt* biPow(BigInt* dest, BigInt* a, BigInt* b);

#endif