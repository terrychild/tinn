#include "lib/crypto/ec.h"
#include "lib/mem/allocator.h"
#include "lib/maths/bigint.h"
#include "lib/log.h"

static EllipticCurvePoint* newPoint(Allocator* allocator, BigInt* p) {
    EllipticCurvePoint* point = allocate(allocator, sizeof(EllipticCurvePoint));
    point->x = biNew(allocator);
    biSetMod(point->x, p);
    point->y = biNew(allocator);
    biSetMod(point->y, p);
    return point;
}

static EllipticCurvePoint* clonePoint(Allocator* allocator, const EllipticCurvePoint* source) {
    EllipticCurvePoint* dest = allocate(allocator, sizeof(EllipticCurvePoint));
    dest->x = biClone(allocator, source->x);
    dest->y = biClone(allocator, source->y);
    dest->is_infinity = source->is_infinity;
    return dest;
}

EllipticCurve* ecNew(Allocator* allocator, const char* a, const char* b, const char* p, const char* x, const char* y) {
    EllipticCurve* ec = allocate(allocator, sizeof(EllipticCurve));
    ec->a = biFromHex(allocator, a);
    ec->b = biFromHex(allocator, b);
    ec->p = biFromHex(allocator, p);
    ec->g = allocate(allocator, sizeof(EllipticCurvePoint));
    ec->g->x = biFromHex(allocator, x);
    ec->g->y = biFromHex(allocator, y);
    return ec;
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

static EllipticCurvePoint* add(Allocator* allocator, const EllipticCurve* ec, const EllipticCurvePoint* p, const EllipticCurvePoint* q) {
    if (p->is_infinity) {
        return clonePoint(allocator, q);
    }
    if (q->is_infinity) {
        return clonePoint(allocator, p);
    }

    BigInt* temp = biNew(allocator);
    biSetMod(temp, ec->p);

    if (biCmp(p->x, q->x)==0 && biIsZero(biAdd(biCopy(temp, p->y), q->y))) {
        EllipticCurvePoint* point = newPoint(allocator, ec->p);
        point->is_infinity = true;
        return point;
    }

    BigInt* s = biNew(allocator);
    biSetMod(s, ec->p);
    if (biCmp(p->x, q->x)==0 && biCmp(p->y, q->y)==0) {
        // double: s = ((3 * p.x * p.x) + e.a) * inverse(2 * p.y, e.p);
        biCopy(s, p->x);
        biPow(s, 2);
        biMul(s, 3);
        biAdd(s, ec->a);

        biCopy(temp, p->y);
        biMul(temp, 2);
        inverse(allocator, temp, ec->p);

        biMul(s, temp);
    } else {
        // add: s = (q.y - p.y) * inverse(q.x - p.x, e.p);
        biCopy(s, q->y);
        biSub(s, p->y);

        biCopy(temp, q->x);
        biSub(temp, p->x);
        inverse(allocator, temp, ec->p);

        biMul(s, temp);
    }

    EllipticCurvePoint* point = newPoint(allocator, ec->p);
    point->is_infinity = false;

    // x = (s * s) - p.x - q.x;
    biCopy(point->x, s);
    biPow(point->x, 2);
    biSub(point->x, p->x);
    biSub(point->x, q->x);

    // y = (s * (p.x - x)) - p.y;
    biCopy(point->y, p->x);
    biSub(point->y, point->x);
    biMul(point->y, s);
    biSub(point->y, p->y);

    return point;
}

static EllipticCurvePoint* mul(Allocator* allocator, const EllipticCurve* ec, const EllipticCurvePoint* p, BigInt* n) {
    EllipticCurvePoint* t = clonePoint(allocator, p);
    for (U64 bit = n->length - 1; bit-- > 0; ) {
        t = add(allocator, ec, t, t);
        if (biBit(n, bit)) {
            t = add(allocator, ec, t, p);
        }
    }
    return t;
}

EllipticCurvePoint* ecPoint(Allocator* allocator, const EllipticCurve* ec, BigInt* n) {
    Allocator* temp_allocator = allocatorNew(0);
    EllipticCurvePoint* point = clonePoint(allocator, mul(temp_allocator, ec, ec->g, n));
    allocatorRelease(temp_allocator);
    return point;
}