#include <stdio.h>

#include "test/expect.h"
#include "lib/mem/allocator.h"
#include "lib/types.h"
#include "lib/bigint.h"
#include "lib/bytes.h"

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

EllipticCurvePoint ecMul(Allocator* allocator, EllipticCurve e, EllipticCurvePoint p1, BigInt* n) {
    EllipticCurvePoint pn = {biClone(allocator, p1.x), biClone(allocator, p1.y), p1.is_infinity};
    for (U64 bit = n->length - 1; bit-- > 0; ) {
        //printf("%lu: %d\n", bit, biBit(n, bit));
        pn = ecAdd(allocator, e, pn, pn);
        if (biBit(n, bit)) {
            pn = ecAdd(allocator, e, pn, p1);
        }
    }
    return pn;
}

void testCrypto(Allocator* allocator) {
    EllipticCurve ec = { .a = biFromInt(allocator, 2), .b = biFromInt(allocator, 2), .p = biFromInt(allocator, 17) };
    EllipticCurvePoint p1 = {biFromInt(allocator, 5), biFromInt(allocator, 1), false};
    EllipticCurvePoint pn = ecAdd(allocator, ec, p1, p1);
    expect("Elliptic Curve double [x]", pn.x->limbs[0], 6);
    expect("Elliptic Curve double [y]", pn.y->limbs[0], 3);
    for (int i=3; i<=21; i++) {
        pn = ecMul(allocator, ec, p1, biFromInt(allocator, i));
        if (pn.is_infinity) {
            printf("Elliptic Curve %dP: infinity\n", i);
        } else {
            printf("Elliptic Curve %dP: (%lu, %lu)\n", i, pn.x->limbs[0], pn.y->limbs[0]);
        }
    }

    ec = (EllipticCurve) {
        .a = biFromHex(allocator, "ffffffff00000001000000000000000000000000fffffffffffffffffffffffc"),
        .b = biFromHex(allocator, "5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b"),
        .p = biFromHex(allocator, "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff")
    };
    p1 = (EllipticCurvePoint) {
        biFromHex(allocator, "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296"),
        biFromHex(allocator, "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5"),
        false
    };
    pn = ecAdd(allocator, ec, p1, p1);
    expect("Elliptic Curve secp256r1 2G x", pn.x, biFromHex(allocator, "7cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978"));
    expect("Elliptic Curve secp256r1 2G y", pn.y, biFromHex(allocator, "7775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1"));

    pn = ecMul(allocator, ec, p1, biFromInt(allocator, 10));
    expect("Elliptic Curve secp256r1 10G x", pn.x, biFromHex(allocator, "cef66d6b2a3a993e591214d1ea223fb545ca6c471c48306e4c36069404c5723f"));
    expect("Elliptic Curve secp256r1 10G y", pn.y, biFromHex(allocator, "878662a229aaae906e123cdd9d3b4c10590ded29fe751eeeca34bbaa44af0773"));

    allocatorDebug(allocator);
    allocatorReset(allocator);
}