#include "lib/crypto/ec.h"
#include "lib/mem/memory.h"
#include "lib/maths/bigint.h"
#include "lib/log.h"

static EllipticCurvePoint* newPoint(Memory* mem) {
    EllipticCurvePoint* point = allocate(mem, sizeof(EllipticCurvePoint));
    point->x = biNew(mem);
    point->y = biNew(mem);
    point->is_infinity = true;
    return point;
}

static EllipticCurvePoint* clonePoint(Memory* mem, const EllipticCurvePoint* source) {
    EllipticCurvePoint* dest = allocate(mem, sizeof(EllipticCurvePoint));
    dest->x = biClone(mem, source->x);
    dest->y = biClone(mem, source->y);
    dest->is_infinity = source->is_infinity;
    return dest;
}

EllipticCurve* ecNew(Memory* mem, const char* a, const char* b, const char* p, const char* x, const char* y) {
    EllipticCurve* ec = allocate(mem, sizeof(EllipticCurve));
    ec->a = biFromHex(mem, a);
    ec->b = biFromHex(mem, b);
    ec->p = biFromHex(mem, p);
    ec->g = allocate(mem, sizeof(EllipticCurvePoint));
    ec->g->x = biFromHex(mem, x);
    ec->g->y = biFromHex(mem, y);
    return ec;
}

static EllipticCurvePoint* add(Memory* mem, const EllipticCurve* ec, const EllipticCurvePoint* p, const EllipticCurvePoint* q) {
    if (p->is_infinity) {
        return clonePoint(mem, q);
    }
    if (q->is_infinity) {
        return clonePoint(mem, p);
    }

    EllipticCurvePoint* point = newPoint(mem);
    ScratchMemory scratch = scratchMemory(mem);

    if (biCmp(p->x, q->x)!=0 || !biIsZero(biAdd(scratch.mem, p->y, q->y, ec->p))) {
        BigInt* s = biNew(scratch.mem);
        BigInt* demoninator = biNew(scratch.mem);
        if (biCmp(p->x, q->x)==0 && biCmp(p->y, q->y)==0) {
            // double: s = ((3 * p.x * p.x) + e.a) * inverse(2 * p.y, e.p);
            biPow(s, p->x, 2, ec->p);
            biMul(s, s, 3, ec->p);
            biAdd(s, s, ec->a, ec->p);

            biMul(demoninator, p->y, 2, ec->p);

        } else {
            // add: s = (q.y - p.y) * inverse(q.x - p.x, e.p);
            biSub(s, q->y, p->y, ec->p);

            biSub(demoninator, q->x, p->x, ec->p);
        }
        biMul(s, s, biMulInv(demoninator, ec->p), ec->p);

        point->is_infinity = false;

        // x = (s * s) - p.x - q.x;
        biPow(point->x, s, 2, ec->p);
        biSub(point->x, point->x, p->x, ec->p);
        biSub(point->x, point->x, q->x, ec->p);

        // y = (s * (p.x - x)) - p.y;
        biSub(point->y, p->x, point->x, ec->p);
        biMul(point->y, point->y, s, ec->p);
        biSub(point->y, point->y, p->y, ec->p);
    }

    scratchRelease(scratch);
    return point;
}

static EllipticCurvePoint* mul(Memory* mem, const EllipticCurve* ec, const EllipticCurvePoint* p, BigInt* n) {
    ScratchMemory scratch = scratchMemory(mem);

    EllipticCurvePoint* t = clonePoint(scratch.mem, p);
    for (U64 bit = n->length - 1; bit-- > 0; ) {
        t = add(scratch.mem, ec, t, t);
        if (biBit(n, bit)) {
            t = add(scratch.mem, ec, t, p);
        }
    }

    EllipticCurvePoint* point = clonePoint(mem, t);
    scratchRelease(scratch);
    return point;
}

EllipticCurvePoint* ecPoint(Memory* mem, const EllipticCurve* ec, BigInt* n) {
    return mul(mem, ec, ec->g, n);
}