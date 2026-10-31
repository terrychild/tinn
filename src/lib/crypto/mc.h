#ifndef LIB_CRYPTO_MC_H
#define LIB_CRYPTO_MC_H

#include "lib/types.h"

typedef struct {
    BigInt* a;
    BigInt* b;
    BigInt* p;
    BigInt* a24;
    BigInt* g;
} MontgomeryCurve;

MontgomeryCurve* mcNew(Allocator* allocator, const char* a, const char* b, const char* p, const char* x);
void xMul(Allocator* allocator, MontgomeryCurve* mc, BigInt* x, BigInt* k);

#endif