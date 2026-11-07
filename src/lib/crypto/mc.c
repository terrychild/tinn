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
    mc->a = biFrom(mem, a);
    mc->b = biFrom(mem, b);
    mc->p = biFrom(mem, p);
    mc->a24 = biNew(mem);
    mc->g = biFrom(mem, x);

    biAdd(mc->a24, mc->a, 2);
    biDiv(mc->a24, 4);

    return mc;
}

static void dbl(Memory* mem, ProjectedPoint* r, const ProjectedPoint* r1, const BigInt* a24, const BigInt* p) {
    BigInt* v1 = biAdd(mem, r1->x, r1->z, p);
    biPow(v1, v1, 2, p);

    BigInt* v2 = biSub(mem, r1->x, r1->z, p);
    biPow(v2, v2, 2, p);

    biMul(r->x, v1, v2, p);

    biSub(v1, v1, v2, p);

    BigInt* v3 = biMul(mem, a24, v1, p);
    biAdd(v3, v3, v2, p);

    biMul(r->z, v1, v3, p);
}

static void add(Memory* scratch_mem, ProjectedPoint* r, const ProjectedPoint* r1, const ProjectedPoint* r2, const BigInt* x, const BigInt* p) {
    BigInt* v0 = biAdd(scratch_mem, r1->x, r1->z, p);

    BigInt* v1 = biSub(scratch_mem, r2->x, r2->z, p);
    biMul(v1, v1, v0, p);

    biSub(v0, r1->x, r1->z, p);

    BigInt* v2 = biAdd(scratch_mem, r2->x, r2->z, p);
    biMul(v2, v2, v0, p);

    biAdd(r->x, v1, v2, p);
    biPow(r->x, r->x, 2, p);

    biSub(r->z, v1, v2, p);
    biPow(r->z, r->z, 2, p);
    biMul(r->z, r->z, x, p);
}

static BigInt* mul(Memory* mem, const MontgomeryCurve* mc, const BigInt* scalar, const BigInt* point) {
    ScratchMemory scratch = scratchMemory(mem);

    ProjectedPoint r0 = {
        .x = biFrom(scratch.mem, point),
        .z = biFrom(scratch.mem, 1)
    };
    ProjectedPoint r1 = {
        .x = biNew(scratch.mem),
        .z = biNew(scratch.mem)
    };
    dbl(scratch.mem, &r1, &r0, mc->a24, mc->p);
    for (U64 bit = scalar->length - 1; bit-- > 0; ) {
        if (biBit(scalar, bit) == 0) {
            add(scratch.mem, &r1, &r0, &r1, point, mc->p);
            dbl(scratch.mem, &r0, &r0, mc->a24, mc->p);
        } else {
            add(scratch.mem, &r0, &r0, &r1, point, mc->p);
            dbl(scratch.mem, &r1, &r1, mc->a24, mc->p);
        }
    }

    BigInt* x = biMod(biMul(biFrom(mem, r0.x), biMulInv(scratch.mem, r0.z, mc->p)), mc->p);
    scratchRelease(scratch);
    return x;
}

BigInt* mcPublicKey(Memory* mem, const MontgomeryCurve* mc, const BigInt* private_key) {
    return mul(mem, mc, private_key, mc->g);
}
BigInt* mc(Memory* mem, const MontgomeryCurve* mc, const BigInt* scalar, const BigInt* point) {
    return mul(mem, mc, scalar, point);
}