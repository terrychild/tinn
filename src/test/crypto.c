#include <stdio.h>

#include "test/expect.h"
#include "lib/mem/allocator.h"
#include "lib/types.h"
#include "lib/bigint.h"

typedef struct {
    BigInt* a;
    BigInt* b;
    BigInt* p;
} EllipticCurve;

typedef struct {
    BigInt* x;
    BigInt* y;
    bool is_infinity;
} EllipticCurvePoint;

BigInt* gcd(Allocator* allocator, BigInt* a, BigInt* b) {
    if (biIsZero(b)) return biClone(allocator, a);
	return gcd(allocator, b, biMod(biNew(allocator), a, b));
}

/*int mod(int a, int mod) {
    int r = a % mod;
    return r < 0 ? r + mod : r;
}*/

struct eea_terms {
	BigInt* r;
	BigInt* s;
	BigInt* t;
};

struct eea_terms eea_recurse(Allocator* allocator, struct eea_terms i2, struct eea_terms i1) {

	if (biIsZero(i1.r)) return i2;

    BigInt* r = biNew(allocator);
    BigInt* s = biNew(allocator);
    BigInt* t = biNew(allocator);

	BigInt* q = biDiv(biNew(allocator), i2.r, i1.r, r);

	return eea_recurse(allocator, i1, (struct eea_terms) {
		.r = r,
		.s = biSub(s, i2.s, biMul(s, q, i1.s)),
		.t = biSub(t, i2.t, biMul(t, q, i1.t))
	});
}

struct eea_terms eea(Allocator* allocator, BigInt* r0, BigInt* r1) {
	return eea_recurse(allocator, (struct eea_terms) {r0, biFromInt(allocator, 1), biFromInt(allocator, 0)}, (struct eea_terms) {r1, biFromInt(allocator, 0), biFromInt(allocator, 1)});
}

BigInt* inverse(Allocator* allocator, BigInt* a, BigInt* mod) {
	struct eea_terms result = eea(allocator, mod, a);

    if (biCmpInt(result.r, 1) != 0) {
        printf("GCD is not 1 so the inverse does not exsis\n");
        return biFromInt(allocator, -1);
    }

    return biMod(result.t, result.t, mod);
}

EllipticCurvePoint ecAdd(Allocator* allocator, EllipticCurve e, EllipticCurvePoint p1, EllipticCurvePoint p2) {
    if (p1.is_infinity) {
        return p2;
    }
    if (p2.is_infinity) {
        return p1;
    }

    BigInt* temp = biNew(allocator);
    biSetMod(temp, e.p);
    if (biCmp(p1.x, p2.x)==0 && biIsZero(biAdd(temp, p1.y, p2.y))) {
        return (EllipticCurvePoint) {biFromInt(allocator, 0), biFromInt(allocator, 0), true};
    }

    BigInt* s = biNew(allocator);
    biSetMod(s, e.p);
    if (biCmp(p1.x, p2.x)==0 && biCmp(p1.y, p2.y)==0) {
        //s = mod(((3 * p1.x * p1.x) + e.a) * inverse(mod(2 * p1.y, e.p), e.p), e.p);
        biMul(s, p1.x, p1.x);
        biMul(s, s, biFromInt(allocator, 3));
        biAdd(s, s, e.a);
        biMul(s, s, inverse(allocator, biMul(temp, biFromInt(allocator, 2), p1.y), e.p));
    } else {
        //s = mod((p2.y - p1.y) * inverse(mod(p2.x - p1.x, e.p), e.p), e.p);
        biSub(s, p2.y, p1.y);
        biSub(temp, p2.x, p1.x);
        biMul(s, s, inverse(allocator, biSub(temp, p2.x, p1.x), e.p));
    }
    //int x = mod((s * s) - p1.x - p2.x, e.p);
    BigInt* x = biNew(allocator);
    biSetMod(x, e.p);
    biMul(x, s, s);
    biSub(x, x, p1.x);
    biSub(x, x, p2.x);

    //int y = mod((s * (p1.x - x)) - p1.y, e.p);
    BigInt* y = biNew(allocator);
    biSetMod(y, e.p);
    biSub(y, p1.x, x);
    biMul(y, y, s);
    biSub(y, y, p1.y);

    return (EllipticCurvePoint) {x, y, false};
}

void testCrypto(Allocator* allocator) {
    EllipticCurve ec = { .a = biFromInt(allocator, 2), .b = biFromInt(allocator, 2), .p = biFromInt(allocator, 17) };
    EllipticCurvePoint p1 = {biFromInt(allocator, 5), biFromInt(allocator, 1), false};
    EllipticCurvePoint pn = ecAdd(allocator, ec, p1, p1);
    expect("Elliptic Curve double [x]", pn.x->limbs[0], 6);
    expect("Elliptic Curve double [y]", pn.y->limbs[0], 3);
    EllipticCurvePoint pl = pn;
    for (int i=3; i<=21; i++) {
        pn = ecAdd(allocator, ec, p1, pl);
        if (pn.is_infinity) {
            printf("Elliptic Curve %dP: infinity\n", i);
        } else {
            printf("Elliptic Curve %dP: (%lu, %lu)\n", i, pn.x->limbs[0], pn.y->limbs[0]);
        }
        pl = pn;
    }

    allocatorDebug(allocator);
    allocatorReset(allocator);
}