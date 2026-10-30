#ifndef LIB_CRYPTO_H
#define LIB_CRYPTO_H

#include "lib/types.h"

typedef struct {
    BigInt* x;
    BigInt* y;
    bool is_infinity;
} EllipticCurvePoint;

typedef struct {
    BigInt* a;
    BigInt* b;
    BigInt* p;
    EllipticCurvePoint* g;
} EllipticCurve;

EllipticCurve* ecNew(Allocator* allocator, const char* a, const char* b, const char* p, const char* x, const char* y);
EllipticCurvePoint* ecPoint(Allocator* allocator, const EllipticCurve* ec, BigInt* n);

#endif