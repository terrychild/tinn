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

static EllipticCurvePoint* clonePoint(Allocator* allocator, EllipticCurvePoint* source) {
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


struct eea_terms {
	BigInt* r;
	BigInt* s;
	BigInt* t;
};

struct eea_terms eea_recurse(Allocator* allocator, struct eea_terms i2, struct eea_terms i1) {

	if (biIsZero(i1.r)) return i2;

    struct eea_terms terms = {
        .r = biNew(allocator),
        .s = biNew(allocator),
        .t = biNew(allocator)
    };

    BigInt* q = biClone(allocator, i2.r);
	biDivRem(q, i1.r, terms.r);

    biCopy(terms.s, i2.s);
    biSub(terms.s, biMul(biClone(allocator, q), i1.s));

    biCopy(terms.t, i2.t);
    biSub(terms.t, biMul(biClone(allocator, q), i1.t));

	return eea_recurse(allocator, i1, terms);
}

struct eea_terms eea(Allocator* allocator, BigInt* r0, BigInt* r1) {
	return eea_recurse(allocator, (struct eea_terms) {r0, biFromInt(allocator, 1), biFromInt(allocator, 0)}, (struct eea_terms) {r1, biFromInt(allocator, 0), biFromInt(allocator, 1)});
}

BigInt* inverse(Allocator* allocator, BigInt* a, BigInt* mod) {
	struct eea_terms result = eea(allocator, mod, a);

    PANIC_IF(biCmp(result.r, 1) != 0, "GCD is not 1 so the inverse does not exsis\n");

    biCopy(a, result.t);
    return biMod(a, mod);
}

static EllipticCurvePoint* ecAdd(Allocator* allocator, const EllipticCurve* ec, const EllipticCurvePoint* p, const EllipticCurvePoint* q) {
    if (p->is_infinity) {
        return q;
    }
    if (q->is_infinity) {
        return p;
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
        // double
        //s = mod(((3 * p.x * p.x) + e.a) * inverse(mod(2 * p.y, e.p), e.p), e.p);
        biCopy(s, p->x);
        biPow(s, 2);
        biMul(s, 3);
        biAdd(s, ec->a);

        biCopy(temp, p->y);
        biMul(temp, 2);
        inverse(allocator, temp, ec->p);

        biMul(s, temp);
    } else {
        // add
        //s = mod((q.y - p.y) * inverse(mod(q.x - p.x, e.p), e.p), e.p);
        biCopy(s, q->y);
        biSub(s, p->y);

        biCopy(temp, q->x);
        biSub(temp, p->x);
        inverse(allocator, temp, ec->p);

        biMul(s, temp);
    }

    EllipticCurvePoint* point = newPoint(allocator, ec->p);
    point->is_infinity = false;

    //int x = mod((s * s) - p.x - q.x, e.p);
    biCopy(point->x, s);
    biPow(point->x, 2);
    biSub(point->x, p->x);
    biSub(point->x, q->x);

    //int y = mod((s * (p1.x - x)) - p1.y, e.p);
    biCopy(point->y, p->x);
    biSub(point->y, point->x);
    biMul(point->y, s);
    biSub(point->y, p->y);

    return point;
}

static EllipticCurvePoint* ecMul(Allocator* allocator, const EllipticCurve* ec, const EllipticCurvePoint* p, BigInt* n) {
    EllipticCurvePoint* t = clonePoint(allocator, p);
    for (U64 bit = n->length - 1; bit-- > 0; ) {
        t = ecAdd(allocator, ec, t, t);
        if (biBit(n, bit)) {
            t = ecAdd(allocator, ec, t, p);
        }
    }
    return t;
}

EllipticCurvePoint* ecPoint(Allocator* allocator, const EllipticCurve* ec, BigInt* n) {
    return ecMul(allocator, ec, ec->g, n);
}