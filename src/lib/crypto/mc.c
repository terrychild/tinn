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

static BigInt* inverse(Allocator* allocator, BigInt* a, BigInt* mod) {
    BigInt* old_t = biFromInt(allocator, 0);
    BigInt* t = biFromInt(allocator, 1);
    BigInt* new_t = biNew(allocator);

    BigInt* old_r = biClone(allocator, mod);
    BigInt* r = biClone(allocator, a);
    BigInt* new_r = biNew(allocator);

    BigInt* q = biNew(allocator);

    while (!biIsZero(r)) {
        biDivRem(biCopy(q, old_r), r, new_r);
        biSub(biCopy(new_t, old_t), biMul(q, t));

        biCopy(old_t, t);
        biCopy(t, new_t);
        biCopy(old_r, r);
        biCopy(r, new_r);
    }

    PANIC_IF(biCmp(old_r, 1) != 0, "GCD is not 1 so the inverse does not exsis\n");
    if (old_t->negative) {
        biAdd(old_t, mod);
    }
    return biCopy(a, old_t);
}

void xMul(Allocator* allocator, MontgomeryCurve* mc, BigInt* x, BigInt* k) {
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
    biMul(x, inverse(allocator, r0.z, mc->p));
    biMod(x, mc->p);
}