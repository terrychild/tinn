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

int biCmpBi(const BigInt* a, const BigInt* b);
int biCmpInt(const BigInt* a, I64 b);
#define biCmp(a, b) _Generic((b), \
		BigInt*: biCmpBi, \
        const BigInt*: biCmpBi, \
        default: biCmpInt  \
    )(a, b)

BigInt* biShiftLeft(BigInt* bi, U64 count);
BigInt* biShiftRight(BigInt* bi, U64 count);

BigInt* biAddBi(BigInt* a, const BigInt* b);
BigInt* biAddInt(BigInt* a, I64 b);
#define biAdd(a, b) _Generic((b), \
		BigInt*: biAddBi, \
        const BigInt*: biAddBi, \
        default: biAddInt  \
    )(a, b)

BigInt* biSubBi(BigInt* a, const BigInt* b);
BigInt* biSubInt(BigInt* a, I64 b);
#define biSub(a, b) _Generic((b), \
		BigInt*: biSubBi, \
        const BigInt*: biSubBi, \
        default: biSubInt  \
    )(a, b)

BigInt* biMulBi(BigInt* a, const BigInt* b);
BigInt* biMulInt(BigInt* a, I64 b);
#define biMul(a, b) _Generic((b), \
		BigInt*: biMulBi, \
        const BigInt*: biMulBi, \
        default: biMulInt  \
    )(a, b)

BigInt* biDivBi(BigInt* a, const BigInt* b);
BigInt* biDivInt(BigInt* a, I64 b);
#define biDiv(a, b) _Generic((b), \
		BigInt*: biDivBi, \
        const BigInt*: biDivBi, \
        default: biDivInt  \
    )(a, b)

BigInt* biDivRemBi(BigInt* a, const BigInt* b, BigInt* r);
BigInt* biDivRemInt(BigInt* a, I64 b, BigInt* r);
#define biDivRem(a, b, r) _Generic((b), \
		BigInt*: biDivRemBi, \
        const BigInt*: biDivRemBi, \
        default: biDivRemInt  \
    )(a, b, r)

BigInt* biModBi(BigInt* a, const BigInt* b);
BigInt* biModInt(BigInt* a, I64 b);
#define biMod(a, b) _Generic((b), \
		BigInt*: biModBi, \
        const BigInt*: biModBi, \
        default: biModInt  \
    )(a, b)

BigInt* biPowBi(BigInt* a, const BigInt* b);
BigInt* biPowInt(BigInt* a, I64 b);
#define biPow(a, b) _Generic((b), \
		BigInt*: biPowBi, \
        const BigInt*: biPowBi, \
        default: biPowInt  \
    )(a, b)

BigInt* biMulInv(BigInt* a, const BigInt* mod);

#endif