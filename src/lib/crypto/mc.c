#include "lib/crypto/mc.h"
#include "lib/mem/memory.h"
#include "lib/maths/bigint.h"
#include "lib/log.h"

typedef struct {
    BigInt* x;
    BigInt* z;
} ProjectedPoint;

MontgomeryCurve* mcNew(Memory* mem, const char* a, const char* b, const char* p, const char* x) {
    MontgomeryCurve* mc = allocate(mem, sizeof(MontgomeryCurve));
    mc->a = biFromHex(mem, a);
    mc->b = biFromHex(mem, b);
    mc->p = biFromHex(mem, p);
    mc->a24 = biNew(mem);
    mc->g = biFromHex(mem, x);

    biSet(mc->a24, mc->a);
    biAdd(mc->a24, 2);
    biDiv(mc->a24, 4);

    return mc;
}

static void dbl(Memory* mem, ProjectedPoint* r, ProjectedPoint* r1, BigInt* a24, BigInt* p) {
    BigInt* v1 = biNew(mem);
    biSetMod(v1, p);
    biSet(v1, r1->x);
    biAdd(v1, r1->z);
    biPow(v1, 2);

    BigInt* v2 = biNew(mem);
    biSetMod(v2, p);
    biSet(v2, r1->x);
    biSub(v2, r1->z);
    biPow(v2, 2);

    biSet(r->x, v1);
    biMul(r->x, v2);

    biSub(v1, v2);

    BigInt* v3 = biNew(mem);
    biSetMod(v3, p);
    biSet(v3, a24);
    biMul(v3, v1);
    biAdd(v3, v2);

    biSet(r->z, v1);
    biMul(r->z, v3);
}

static void add(Memory* mem, ProjectedPoint* r, ProjectedPoint* r1, ProjectedPoint* r2, BigInt* x, BigInt* p) {
    BigInt* v0 = biNew(mem);
    biSetMod(v0, p);
    biSet(v0, r1->x);
    biAdd(v0, r1->z);

    BigInt* v1 = biNew(mem);
    biSetMod(v1, p);
    biSet(v1, r2->x);
    biSub(v1, r2->z);
    biMul(v1, v0);

    biSet(v0, r1->x);
    biSub(v0, r1->z);

    BigInt* v2 = biNew(mem);
    biSetMod(v2, p);
    biSet(v2, r2->x);
    biAdd(v2, r2->z);
    biMul(v2, v0);

    biSet(r->x, v1);
    biAdd(r->x, v2);
    biPow(r->x, 2);

    biSet(r->z, v1);
    biSub(r->z, v2);
    biPow(r->z, 2);
    biMul(r->z, x);
}

static void mul(Memory* mem, BigInt* x, MontgomeryCurve* mc, BigInt* k) {
    ProjectedPoint r0 = {
        .x = biClone(mem, mc->g),
        .z = biFromInt(mem, 1)
    };
    ProjectedPoint r1 = {
        .x = biNew(mem),
        .z = biNew(mem)
    };
    biSetMod(r0.x, mc->p);
    biSetMod(r0.z, mc->p);
    biSetMod(r1.x, mc->p);
    biSetMod(r1.z, mc->p);
    dbl(mem, &r1, &r0, mc->a24, mc->p);
    for (U64 bit = k->length - 1; bit-- > 0; ) {
        if (biBit(k, bit) == 0) {
            add(mem, &r1, &r0, &r1, mc->g, mc->p);
            dbl(mem, &r0, &r0, mc->a24, mc->p);
        } else {
            add(mem, &r0, &r0, &r1, mc->g, mc->p);
            dbl(mem, &r1, &r1, mc->a24, mc->p);
        }
    }

    biSet(x, r0.x);
    biMul(x, biMulInv(r0.z, mc->p));
    biMod(x, mc->p);
}

void mcKey(BigInt* point, MontgomeryCurve* mc, BigInt* n) {
    Memory* mem = memoryNew(0);
    mul(mem, point, mc, n);
    memoryRelease(mem);
}