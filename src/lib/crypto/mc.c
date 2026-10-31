#include "lib/crypto/mc.h"
#include "lib/mem/allocator.h"
#include "lib/maths/bigint.h"
#include "lib/log.h"

typedef struct {
    BigInt* x;
    BigInt* z;
} ProjectedPoint;

MontgomeryCurve* mcNew(Allocator* allocator, const char* a, const char* b, const char* p, const char* x) {
    MontgomeryCurve* mc = allocate(allocator, sizeof(MontgomeryCurve));
    mc->a = biFromHex(allocator, a);
    mc->b = biFromHex(allocator, b);
    mc->p = biFromHex(allocator, p);
    mc->a24 = biNew(allocator);
    mc->g = biFromHex(allocator, x);

    biCopy(mc->a24, mc->a);
    biAdd(mc->a24, 2);
    biDiv(mc->a24, 4);

    return mc;
}

void dbl(Allocator* allocator, ProjectedPoint* r, ProjectedPoint* r1, BigInt* a24, BigInt* p) {
    BigInt* v1 = biNew(allocator);
    biSetMod(v1, p);
    biCopy(v1, r1->x);
    biAdd(v1, r1->z);
    biPow(v1, 2);

    BigInt* v2 = biNew(allocator);
    biSetMod(v2, p);
    biCopy(v2, r1->x);
    biSub(v2, r1->z);
    biPow(v2, 2);

    biCopy(r->x, v1);
    biMul(r->x, v2);

    biSub(v1, v2);

    BigInt* v3 = biNew(allocator);
    biSetMod(v3, p);
    biCopy(v3, a24);
    biMul(v3, v1);
    biAdd(v3, v2);

    biCopy(r->z, v1);
    biMul(r->z, v3);
}

void add(Allocator* allocator, ProjectedPoint* r, ProjectedPoint* r1, ProjectedPoint* r2, BigInt* x, BigInt* p) {
    BigInt* v0 = biNew(allocator);
    biSetMod(v0, p);
    biCopy(v0, r1->x);
    biAdd(v0, r1->z);

    BigInt* v1 = biNew(allocator);
    biSetMod(v1, p);
    biCopy(v1, r2->x);
    biSub(v1, r2->z);
    biMul(v1, v0);

    biCopy(v0, r1->x);
    biSub(v0, r1->z);

    BigInt* v2 = biNew(allocator);
    biSetMod(v2, p);
    biCopy(v2, r2->x);
    biAdd(v2, r2->z);
    biMul(v2, v0);

    biCopy(r->x, v1);
    biAdd(r->x, v2);
    biPow(r->x, 2);

    biCopy(r->z, v1);
    biSub(r->z, v2);
    biPow(r->z, 2);
    biMul(r->z, x);
}

static void mul(Allocator* allocator, BigInt* x, MontgomeryCurve* mc, BigInt* k) {
    ProjectedPoint r0 = {
        .x = biClone(allocator, mc->g),
        .z = biFromInt(allocator, 1)
    };
    ProjectedPoint r1 = {
        .x = biNew(allocator),
        .z = biNew(allocator)
    };
    biSetMod(r0.x, mc->p);
    biSetMod(r0.z, mc->p);
    biSetMod(r1.x, mc->p);
    biSetMod(r1.z, mc->p);
    dbl(allocator, &r1, &r0, mc->a24, mc->p);
    for (U64 bit = k->length - 1; bit-- > 0; ) {
        if (biBit(k, bit) == 0) {
            add(allocator, &r1, &r0, &r1, mc->g, mc->p);
            dbl(allocator, &r0, &r0, mc->a24, mc->p);
        } else {
            add(allocator, &r0, &r0, &r1, mc->g, mc->p);
            dbl(allocator, &r1, &r1, mc->a24, mc->p);
        }
    }

    biCopy(x, r0.x);
    biMul(x, biMulInv(r0.z, mc->p));
    biMod(x, mc->p);
}

void mcKey(BigInt* point, MontgomeryCurve* mc, BigInt* n) {
    Allocator* allocator = allocatorNew(0);
    mul(allocator, point, mc, n);
    allocatorRelease(allocator);
}