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

MontgomeryCurve* mcNew(Memory* mem, const char* a, const char* b, const char* p, const char* x);
BigInt* mcKey(Memory* mem, const MontgomeryCurve* mc, const BigInt* n);

#endif