#include "lib/crypto/ec.h"
#include "lib/mem/memory.h"
#include "lib/maths/bigint.h"
#include "lib/log.h"

static EllipticCurvePoint* copyPoint(EllipticCurvePoint* dest, const EllipticCurvePoint* source) {
    biSet(dest->x, source->x);
    biSet(dest->y, source->y);
    dest->is_infinity = source->is_infinity;
    return dest;
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

EllipticCurvePoint* ecNewPoint(Memory* mem, const EllipticCurve* ec) {
    EllipticCurvePoint* point = allocate(mem, sizeof(EllipticCurvePoint));
    point->x = biNew(mem);
    biSetMod(point->x, ec->p);
    point->y = biNew(mem);
    biSetMod(point->y, ec->p);
    point->is_infinity = true;
    return point;
}

static EllipticCurvePoint* add(Memory* mem, const EllipticCurve* ec, const EllipticCurvePoint* p, const EllipticCurvePoint* q) {
    if (p->is_infinity) {
        return clonePoint(mem, q);
    }
    if (q->is_infinity) {
        return clonePoint(mem, p);
    }

    BigInt* temp = biNew(mem);
    biSetMod(temp, ec->p);

    if (biCmp(p->x, q->x)==0 && biIsZero(biAdd(biSet(temp, p->y), q->y))) {
        return ecNewPoint(mem, ec);
    }

    BigInt* s = biNew(mem);
    biSetMod(s, ec->p);
    if (biCmp(p->x, q->x)==0 && biCmp(p->y, q->y)==0) {
        // double: s = ((3 * p.x * p.x) + e.a) * inverse(2 * p.y, e.p);
        biSet(s, p->x);
        biPow(s, 2);
        biMul(s, 3);
        biAdd(s, ec->a);

        biSet(temp, p->y);
        biMul(temp, 2);
        biMulInv(temp, ec->p);

        biMul(s, temp);
    } else {
        // add: s = (q.y - p.y) * inverse(q.x - p.x, e.p);
        biSet(s, q->y);
        biSub(s, p->y);

        biSet(temp, q->x);
        biSub(temp, p->x);
        biMulInv(temp, ec->p);

        biMul(s, temp);
    }

    EllipticCurvePoint* point = ecNewPoint(mem, ec);
    point->is_infinity = false;

    // x = (s * s) - p.x - q.x;
    biSet(point->x, s);
    biPow(point->x, 2);
    biSub(point->x, p->x);
    biSub(point->x, q->x);

    // y = (s * (p.x - x)) - p.y;
    biSet(point->y, p->x);
    biSub(point->y, point->x);
    biMul(point->y, s);
    biSub(point->y, p->y);

    return point;
}

static EllipticCurvePoint* mul(Memory* mem, const EllipticCurve* ec, const EllipticCurvePoint* p, BigInt* n) {
    EllipticCurvePoint* t = clonePoint(mem, p);
    for (U64 bit = n->length - 1; bit-- > 0; ) {
        t = add(mem, ec, t, t);
        if (biBit(n, bit)) {
            t = add(mem, ec, t, p);
        }
    }
    return t;
}

void ecPoint(EllipticCurvePoint* point, const EllipticCurve* ec, BigInt* n) {
    Memory* mem = memoryNew(0);
    copyPoint(point, mul(mem, ec, ec->g, n));
    memoryRelease(mem);
}