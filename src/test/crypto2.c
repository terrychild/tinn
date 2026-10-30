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

typedef struct {
    BigInt* a;
    BigInt* b;
    BigInt* p;
    BigInt* a24;
    BigInt* g;
} MontgomeryCurve;

typedef struct {
    BigInt* x;
    BigInt* z;
} ProjectedPoint;

void xDbl(Allocator* allocator, ProjectedPoint* r, ProjectedPoint* r1, BigInt* a24, BigInt* p) {
    BigInt* v1 = biNew(allocator);
    biSetMod(v1, p);
    biAdd(v1, r1->x, r1->z);
    biMul(v1, v1, v1);

    BigInt* v2 = biNew(allocator);
    biSetMod(v2, p);
    biSub(v2, r1->x, r1->z);
    biMul(v2, v2, v2);

    biMul(r->x, v1, v2);

    biSub(v1, v1, v2);

    BigInt* v3 = biNew(allocator);
    biSetMod(v3, p);
    biMul(v3, a24, v1);
    biAdd(v3, v3, v2);

    biMul(r->z, v1, v3);
}

void xAdd(Allocator* allocator, ProjectedPoint* r, ProjectedPoint* r1, ProjectedPoint* r2, BigInt* x, BigInt* p) {
    BigInt* v0 = biNew(allocator);
    biSetMod(v0, p);
    biAdd(v0, r1->x, r1->z);

    BigInt* v1 = biNew(allocator);
    biSetMod(v1, p);
    biSub(v1, r2->x, r2->z);
    biMul(v1, v1, v0);

    biSub(v0, r1->x, r1->z);

    BigInt* v2 = biNew(allocator);
    biSetMod(v2, p);
    biAdd(v2, r2->x, r2->z);
    biMul(v2, v2, v0);

    biAdd(r->x, v1, v2);
    biMul(r->x, r->x, r->x);

    biSub(r->z, v1, v2);
    biMul(r->z, r->z, r->z);
    biMul(r->z, x, r->z);
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
    xDbl(allocator, &r1, &r0, mc->a24, mc->p);
    for (U64 bit = k->length - 1; bit-- > 0; ) {
        if (biBit(k, bit) == 0) {
            xAdd(allocator, &r1, &r0, &r1, mc->g, mc->p);
            xDbl(allocator, &r0, &r0, mc->a24, mc->p);
        } else {
            xAdd(allocator, &r0, &r0, &r1, mc->g, mc->p);
            xDbl(allocator, &r1, &r1, mc->a24, mc->p);
        }
    }
    //printf("x\n");
    //hexDump(biToBESlice(r0.x));
    biMul(x, r0.x, inverse(allocator, r0.z, mc->p));
    biMod(x, x, mc->p);
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

    MontgomeryCurve mc = {
        .a = biFromHex(allocator, "76d06"),
        .b = biFromHex(allocator, "1"),
        .p = biFromHex(allocator, "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed"),
        .g = biFromHex(allocator, "9"),
        .a24 = biNew(allocator)
    };
    biAdd(mc.a24, mc.a, biFromInt(allocator, 2));
    biDiv(mc.a24, mc.a24, biFromInt(allocator, 4), biNew(allocator));

    BigInt* x = biNew(allocator);
    xMul(allocator, &mc, x, biFromInt(allocator, 2));
    expect("Elliptic Curve 25519 2G", x, biFromHex(allocator, "20d342d51873f1b7d9750c687d1571148f3f5ced1e350b5c5cae469cdd684efb"));
    xMul(allocator, &mc, x, biFromInt(allocator, 3));
    expect("Elliptic Curve 25519 3G", x, biFromHex(allocator, "1c12bc1a6d57abe645534d91c21bba64f8824e67621c0859c00a03affb713c12"));
    xMul(allocator, &mc, x, biFromInt(allocator, 10));
    expect("Elliptic Curve 25519 10G", x, biFromHex(allocator, "41eda655b159060471fb4ce5d7cb3fe43ee51843d2080e0383ce42892c3a9c7b"));
    xMul(allocator, &mc, x, biFromInt(allocator, 256));
    expect("Elliptic Curve 25519 256G", x, biFromHex(allocator, "4300017536976a742ec8747f7505cd6bc80e610d669acab1a1eed36f680d98e8"));

    xMul(allocator, &mc, x, biFromHex(allocator, "10000000"));
    expect("Elliptic Curve 25519 0x10000000", x, biFromHex(allocator, "6893cedbcf053a23b7d5676bf577dbc309d25c8525a0f2f5036b67f0967fa084"));

    xMul(allocator, &mc, x, biFromHex(allocator, "FFFFFFFF"));
    expect("Elliptic Curve 25519 0xFFFFFFFF", x, biFromHex(allocator, "08de23ae492b2c0a8b7c7e1f216501ba25bea603a30f5fd0c4bbac77fd5fec82"));

    xMul(allocator, &mc, x, biFromHex(allocator, "100000000"));
    expect("Elliptic Curve 25519 0x100000000", x, biFromHex(allocator, "06a8e0bfab511d23d72ddc7e8b0d4e3561183208d3d7886c269a92ca1de7dc30"));

    xMul(allocator, &mc, x, biFromHex(allocator, "1000000000000000"));
    expect("Elliptic Curve 25519 0x1000000000000000", x, biFromHex(allocator, "733350c17acebbbafc697ea8b5f03b354bf7ae172f54e86e883b59d150d1b370"));
    xMul(allocator, &mc, x, biFromHex(allocator, "6A2CB91DA5FB77B12A99C0EB872F4CDF4566B25172C1163C7DA518730A6D0770"));
    expect("Elliptic Curve 25519", x, biFromHex(allocator, "6a4e9baa8ea9a4ebf41a38260d3abf0d5af73eb4dc7d8b7454a7308909f02085"));
    xMul(allocator, &mc, x, biFromHex(allocator, "4242424242424242424242424242424242424242424242424242424242424240"));
    expect("Elliptic Curve 25519", x, biFromHex(allocator, "72f493999c4dd1279321ae3a50c1cc1fe776aa283360727ed5fb10e02b442c13"));


    allocatorDebug(allocator);
    allocatorReset(allocator);
}