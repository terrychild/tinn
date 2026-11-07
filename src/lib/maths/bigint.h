#ifndef LIB_MATHS_BIGINT_H
#define LIB_MATHS_BIGINT_H

#include <stdlib.h>
#include "lib/macros.h"
#include "lib/types.h"

struct BigInt {
    Memory* mem;
	U64 size;
	U64 length;
	U64* limbs;
	bool negative;
};

// === make new BigInt ===

BigInt* biNew(Memory* mem);

BigInt* biFromBi(Memory* mem, const BigInt* source);
BigInt* biFromInt(Memory* mem, I64 i);
BigInt* biFromStr(Memory* mem, const char* str);
BigInt* biFromBE(Memory* mem, const U8* bytes, U64 count);
BigInt* biFromBESlice(Memory* mem, const Slice slice);
BigInt* biFromLE(Memory* mem, const U8* bytes, U64 count);
BigInt* biFromLESlice(Memory* mem, const Slice slice);

#define biFrom(mem, source) _Generic((source), \
    const BigInt*: biFromBi, \
    BigInt*: biFromBi, \
    const char*: biFromStr, \
    char*: biFromStr, \
    const Slice: biFromBESlice, \
    Slice: biFromBESlice, \
    default: biFromInt  \
)(mem, source)

// === input ===

BigInt* biSetBi(BigInt* dest, const BigInt* source);
BigInt* biSetInt(BigInt* bi, I64 i);
BigInt* biSetStr(BigInt* bi, const char* str);
BigInt* biSetBE(BigInt* bi, const U8* bytes, U64 count);
BigInt* biSetBESlice(BigInt* bi, const Slice slice);
BigInt* biSetLE(BigInt* bi, const U8* bytes, U64 count);
BigInt* biSetLESlice(BigInt* bi, const Slice slice);

#define biSet(dest, source) _Generic((source), \
    const BigInt*: biSetBi, \
    BigInt*: biSetBi, \
    const char*: biSetStr, \
    char*: biSetStr, \
    const Slice: biSetBESlice, \
    Slice: biSetBESlice, \
    default: biSetInt  \
)(dest, source)

// === output ===

Slice biToBESlice(Memory* mem, const BigInt* bi);
Slice biToLESlice(Memory* mem, const BigInt* bi);
char* biToHexStr(Memory* mem, const BigInt* bi);

// === comparison ===

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

// === shift left ===
BigInt* biShiftLeft_bb(BigInt* dest, const BigInt* source, U64 count);
BigInt* biShiftLeft_mb(Memory* mem, const BigInt* source, U64 count);

#define biShiftLeft_dispatch(dest, source, count) _Generic((dest), \
    BigInt*: biShiftLeft_bb, \
    Memory*: biShiftLeft_mb \
)(dest, source, count)

#define biShiftLeft_2(dest, count)         biShiftLeft_dispatch(dest, dest, count)
#define biShiftLeft_3(dest, source, count) biShiftLeft_dispatch(dest, source, count)

#define biShiftLeft(...) CONCAT(biShiftLeft_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

// === shift right ===
BigInt* biShiftRight_bb(BigInt* dest, const BigInt* source, U64 count);
BigInt* biShiftRight_mb(Memory* mem, const BigInt* source, U64 count);

#define biShiftRight_dispatch(dest, source, count) _Generic((dest), \
    BigInt*: biShiftRight_bb, \
    Memory*: biShiftRight_mb \
)(dest, source, count)

#define biShiftRight_2(dest, count)         biShiftRight_dispatch(dest, dest, count)
#define biShiftRight_3(dest, source, count) biShiftRight_dispatch(dest, source, count)

#define biShiftRight(...) CONCAT(biShiftRight_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

// === addition ===

BigInt* biAdd_bbb(BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod);
BigInt* biAdd_bbi(BigInt* dest, const BigInt* a, I64 b, const BigInt* mod);
BigInt* biAdd_bib(BigInt* dest, I64 a, const BigInt* b, const BigInt* mod);
BigInt* biAdd_bii(BigInt* dest, I64 a, I64 b, const BigInt* mod);
BigInt* biAdd_mbb(Memory* mem, const BigInt* a, const BigInt* b, const BigInt* mod);
BigInt* biAdd_mbi(Memory* mem, const BigInt* a, I64 b, const BigInt* mod);
BigInt* biAdd_mib(Memory* mem, I64 a, const BigInt* b, const BigInt* mod);
BigInt* biAdd_mii(Memory* mem, I64 a, I64 b, const BigInt* mod);

#define biAdd_dispatch(dest, a, b, mod) _Generic((dest), \
    BigInt*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biAdd_bbb, \
            BigInt*: biAdd_bbb, \
            default: biAdd_bbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biAdd_bbb, \
            BigInt*: biAdd_bbb, \
            default: biAdd_bbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biAdd_bib, \
            BigInt*: biAdd_bib, \
            default: biAdd_bii  \
        )  \
    ), \
    Memory*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biAdd_mbb, \
            BigInt*: biAdd_mbb, \
            default: biAdd_mbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biAdd_mbb, \
            BigInt*: biAdd_mbb, \
            default: biAdd_mbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biAdd_mib, \
            BigInt*: biAdd_mib, \
            default: biAdd_mii  \
        )  \
    ) \
)(dest, a, b, mod)

#define biAdd_2(a, b)            biAdd_dispatch(   a, a, b, NULL)
#define biAdd_3(dest, a, b)      biAdd_dispatch(dest, a, b, NULL)
#define biAdd_4(dest, a, b, mod) biAdd_dispatch(dest, a, b, mod)

#define biAdd(...) CONCAT(biAdd_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

// === subtraction ===

BigInt* biSub_bbb(BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod);
BigInt* biSub_bbi(BigInt* dest, const BigInt* a, I64 b, const BigInt* mod);
BigInt* biSub_bib(BigInt* dest, I64 a, const BigInt* b, const BigInt* mod);
BigInt* biSub_bii(BigInt* dest, I64 a, I64 b, const BigInt* mod);
BigInt* biSub_mbb(Memory* mem, const BigInt* a, const BigInt* b, const BigInt* mod);
BigInt* biSub_mbi(Memory* mem, const BigInt* a, I64 b, const BigInt* mod);
BigInt* biSub_mib(Memory* mem, I64 a, const BigInt* b, const BigInt* mod);
BigInt* biSub_mii(Memory* mem, I64 a, I64 b, const BigInt* mod);

#define biSub_dispatch(dest, a, b, mod) _Generic((dest), \
    BigInt*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biSub_bbb, \
            BigInt*: biSub_bbb, \
            default: biSub_bbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biSub_bbb, \
            BigInt*: biSub_bbb, \
            default: biSub_bbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biSub_bib, \
            BigInt*: biSub_bib, \
            default: biSub_bii  \
        )  \
    ), \
    Memory*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biSub_mbb, \
            BigInt*: biSub_mbb, \
            default: biSub_mbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biSub_mbb, \
            BigInt*: biSub_mbb, \
            default: biSub_mbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biSub_mib, \
            BigInt*: biSub_mib, \
            default: biSub_mii  \
        )  \
    ) \
)(dest, a, b, mod)

#define biSub_2(a, b)            biSub_dispatch(   a, a, b, NULL)
#define biSub_3(dest, a, b)      biSub_dispatch(dest, a, b, NULL)
#define biSub_4(dest, a, b, mod) biSub_dispatch(dest, a, b, mod)

#define biSub(...) CONCAT(biSub_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

// === multiplication ===

BigInt* biMul_bbb(BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod);
BigInt* biMul_bbi(BigInt* dest, const BigInt* a, I64 b, const BigInt* mod);
BigInt* biMul_bib(BigInt* dest, I64 a, const BigInt* b, const BigInt* mod);
BigInt* biMul_bii(BigInt* dest, I64 a, I64 b, const BigInt* mod);
BigInt* biMul_mbb(Memory* mem, const BigInt* a, const BigInt* b, const BigInt* mod);
BigInt* biMul_mbi(Memory* mem, const BigInt* a, I64 b, const BigInt* mod);
BigInt* biMul_mib(Memory* mem, I64 a, const BigInt* b, const BigInt* mod);
BigInt* biMul_mii(Memory* mem, I64 a, I64 b, const BigInt* mod);

#define biMul_dispatch(dest, a, b, mod) _Generic((dest), \
    BigInt*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biMul_bbb, \
            BigInt*: biMul_bbb, \
            default: biMul_bbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biMul_bbb, \
            BigInt*: biMul_bbb, \
            default: biMul_bbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biMul_bib, \
            BigInt*: biMul_bib, \
            default: biMul_bii  \
        )  \
    ), \
    Memory*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biMul_mbb, \
            BigInt*: biMul_mbb, \
            default: biMul_mbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biMul_mbb, \
            BigInt*: biMul_mbb, \
            default: biMul_mbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biMul_mib, \
            BigInt*: biMul_mib, \
            default: biMul_mii  \
        )  \
    ) \
)(dest, a, b, mod)

#define biMul_2(a, b)            biMul_dispatch(   a, a, b, NULL)
#define biMul_3(dest, a, b)      biMul_dispatch(dest, a, b, NULL)
#define biMul_4(dest, a, b, mod) biMul_dispatch(dest, a, b, mod)

#define biMul(...) CONCAT(biMul_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

// === division ===

BigInt* biDiv_bbb(BigInt* dest, const BigInt* a, const BigInt* b, BigInt* r);
BigInt* biDiv_bbi(BigInt* dest, const BigInt* a, I64 b, BigInt* r);
BigInt* biDiv_bib(BigInt* dest, I64 a, const BigInt* b, BigInt* r);
BigInt* biDiv_bii(BigInt* dest, I64 a, I64 b, BigInt* r);
BigInt* biDiv_mbb(Memory* mem, const BigInt* a, const BigInt* b, BigInt* r);
BigInt* biDiv_mbi(Memory* mem, const BigInt* a, I64 b, BigInt* r);
BigInt* biDiv_mib(Memory* mem, I64 a, const BigInt* b, BigInt* r);
BigInt* biDiv_mii(Memory* mem, I64 a, I64 b, BigInt* r);

#define biDiv_dispatch(dest, a, b, r) _Generic((dest), \
    BigInt*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biDiv_bbb, \
            BigInt*: biDiv_bbb, \
            default: biDiv_bbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biDiv_bbb, \
            BigInt*: biDiv_bbb, \
            default: biDiv_bbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biDiv_bib, \
            BigInt*: biDiv_bib, \
            default: biDiv_bii  \
        )  \
    ), \
    Memory*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biDiv_mbb, \
            BigInt*: biDiv_mbb, \
            default: biDiv_mbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biDiv_mbb, \
            BigInt*: biDiv_mbb, \
            default: biDiv_mbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biDiv_mib, \
            BigInt*: biDiv_mib, \
            default: biDiv_mii  \
        )  \
    ) \
)(dest, a, b, r)

#define biDiv_2(a, b)          biDiv_dispatch(   a, a, b, NULL)
#define biDiv_3(dest, a, b)    biDiv_dispatch(dest, a, b, NULL)
#define biDiv_4(dest, a, b, r) biDiv_dispatch(dest, a, b, r)

#define biDiv(...) CONCAT(biDiv_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

// === modulus ===

BigInt* biMod_bbb(BigInt* dest, const BigInt* a, const BigInt* b);
BigInt* biMod_bbi(BigInt* dest, const BigInt* a, I64 b);
BigInt* biMod_bib(BigInt* dest, I64 a, const BigInt* b);
BigInt* biMod_bii(BigInt* dest, I64 a, I64 b);
BigInt* biMod_mbb(Memory* mem, const BigInt* a, const BigInt* b);
BigInt* biMod_mbi(Memory* mem, const BigInt* a, I64 b);
BigInt* biMod_mib(Memory* mem, I64 a, const BigInt* b);
BigInt* biMod_mii(Memory* mem, I64 a, I64 b);

#define biMod_dispatch(dest, a, b) _Generic((dest), \
    BigInt*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biMod_bbb, \
            BigInt*: biMod_bbb, \
            default: biMod_bbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biMod_bbb, \
            BigInt*: biMod_bbb, \
            default: biMod_bbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biMod_bib, \
            BigInt*: biMod_bib, \
            default: biMod_bii  \
        )  \
    ), \
    Memory*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biMod_mbb, \
            BigInt*: biMod_mbb, \
            default: biMod_mbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biMod_mbb, \
            BigInt*: biMod_mbb, \
            default: biMod_mbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biMod_mib, \
            BigInt*: biMod_mib, \
            default: biMod_mii  \
        )  \
    ) \
)(dest, a, b)

#define biMod_2(a, b)          biMod_dispatch(   a, a, b)
#define biMod_3(dest, a, b)    biMod_dispatch(dest, a, b)

#define biMod(...) CONCAT(biMod_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

// === exponentiation ===

BigInt* biPow_bbb(BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod);
BigInt* biPow_bbi(BigInt* dest, const BigInt* a, I64 b, const BigInt* mod);
BigInt* biPow_bib(BigInt* dest, I64 a, const BigInt* b, const BigInt* mod);
BigInt* biPow_bii(BigInt* dest, I64 a, I64 b, const BigInt* mod);
BigInt* biPow_mbb(Memory* mem, const BigInt* a, const BigInt* b, const BigInt* mod);
BigInt* biPow_mbi(Memory* mem, const BigInt* a, I64 b, const BigInt* mod);
BigInt* biPow_mib(Memory* mem, I64 a, const BigInt* b, const BigInt* mod);
BigInt* biPow_mii(Memory* mem, I64 a, I64 b, const BigInt* mod);

#define biPow_dispatch(dest, a, b, mod) _Generic((dest), \
    BigInt*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biPow_bbb, \
            BigInt*: biPow_bbb, \
            default: biPow_bbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biPow_bbb, \
            BigInt*: biPow_bbb, \
            default: biPow_bbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biPow_bib, \
            BigInt*: biPow_bib, \
            default: biPow_bii  \
        )  \
    ), \
    Memory*: _Generic((a), \
        const BigInt*: _Generic((b), \
            const BigInt*: biPow_mbb, \
            BigInt*: biPow_mbb, \
            default: biPow_mbi  \
        ), \
        BigInt*: _Generic((b), \
            const BigInt*: biPow_mbb, \
            BigInt*: biPow_mbb, \
            default: biPow_mbi  \
        ), \
        default: _Generic((b), \
            const BigInt*: biPow_mib, \
            BigInt*: biPow_mib, \
            default: biPow_mii  \
        )  \
    ) \
)(dest, a, b, mod)

#define biPow_2(a, b)            biPow_dispatch(   a, a, b, NULL)
#define biPow_3(dest, a, b)      biPow_dispatch(dest, a, b, NULL)
#define biPow_4(dest, a, b, mod) biPow_dispatch(dest, a, b, mod)

#define biPow(...) CONCAT(biPow_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

// === multiplicative inverse ===

BigInt* biMulInv_bbb(BigInt* dest, const BigInt* a, const BigInt* mod);
BigInt* biMulInv_bbi(BigInt* dest, const BigInt* a, I64 mod);
BigInt* biMulInv_bib(BigInt* dest, I64 a, const BigInt* mod);
BigInt* biMulInv_bii(BigInt* dest, I64 a, I64 mod);
BigInt* biMulInv_mbb(Memory* mem, const BigInt* a, const BigInt* mod);
BigInt* biMulInv_mbi(Memory* mem, const BigInt* a, I64 mod);
BigInt* biMulInv_mib(Memory* mem, I64 a, const BigInt* mod);
BigInt* biMulInv_mii(Memory* mem, I64 a, I64 mod);

#define biMulInv_dispatch(dest, a, mod) _Generic((dest), \
    BigInt*: _Generic((a), \
        const BigInt*: _Generic((mod), \
            const BigInt*: biMulInv_bbb, \
            BigInt*: biMulInv_bbb, \
            default: biMulInv_bbi  \
        ), \
        BigInt*: _Generic((mod), \
            const BigInt*: biMulInv_bbb, \
            BigInt*: biMulInv_bbb, \
            default: biMulInv_bbi  \
        ), \
        default: _Generic((mod), \
            const BigInt*: biMulInv_bib, \
            BigInt*: biMulInv_bib, \
            default: biMulInv_bii  \
        )  \
    ), \
    Memory*: _Generic((a), \
        const BigInt*: _Generic((mod), \
            const BigInt*: biMulInv_mbb, \
            BigInt*: biMulInv_mbb, \
            default: biMulInv_mbi  \
        ), \
        BigInt*: _Generic((mod), \
            const BigInt*: biMulInv_mbb, \
            BigInt*: biMulInv_mbb, \
            default: biMulInv_mbi  \
        ), \
        default: _Generic((mod), \
            const BigInt*: biMulInv_mib, \
            BigInt*: biMulInv_mib, \
            default: biMulInv_mii  \
        )  \
    ) \
)(dest, a, mod)

#define biMulInv_2(a, mod)       biMulInv_dispatch(   a, a, mod)
#define biMulInv_3(dest, a, mod) biMulInv_dispatch(dest, a, mod)

#define biMulInv(...) CONCAT(biMulInv_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

#endif