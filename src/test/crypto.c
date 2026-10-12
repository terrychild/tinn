#include <stdio.h>

#include "test/expect.h"

#include "lib/types.h"

typedef struct {
    int a;
    int b;
    int p;
} EllipticCurve;

typedef struct {
    int x;
    int y;
    bool is_infinity;
} EllipticCurvePoint;

int gcd(int a, int b) {
	if (b == 0) return a;
	return gcd(b, a % b);
}

int mod(int a, int mod) {
    int r = a % mod;
    return r < 0 ? r + mod : r;
}

struct eea_terms {
	int r;
	int s;
	int t;
};

struct eea_terms eea_recurse(struct eea_terms i2, struct eea_terms i1) {

	if (i1.r==0) return i2;

	int q = i2.r / i1.r;
	return eea_recurse(i1, (struct eea_terms) {
		.r = i2.r % i1.r,
		.s = i2.s - q*i1.s,
		.t = i2.t - q*i1.t
	});
}

struct eea_terms eea(int r0, int r1) {
	return eea_recurse((struct eea_terms) {r0, 1, 0}, (struct eea_terms) {r1, 0, 1});
}

int inverse(int a, int mod) {
	struct eea_terms result = eea(mod, a);

	return result.t<0 ? result.t+mod : result.t;
}

EllipticCurvePoint ecAdd(EllipticCurve e, EllipticCurvePoint p1, EllipticCurvePoint p2) {
    if (p1.is_infinity) {
        return p2;
    }
    if (p2.is_infinity) {
        return p1;
    }

    if (p1.x == p2.x && mod(p1.y + p2.y, e.p) == 0) {
        return (EllipticCurvePoint) {0, 0, true};
    }

    int s;
    if (p1.x == p2.x && p1.y == p2.y) {
        s = mod(((3 * p1.x * p1.x) + e.a) * inverse(mod(2 * p1.y, e.p), e.p), e.p);
    } else {
        s = mod((p2.y - p1.y) * inverse(mod(p2.x - p1.x, e.p), e.p), e.p);
    }
    int x = mod((s * s) - p1.x - p2.x, e.p);
    int y = mod((s * (p1.x - x)) - p1.y, e.p);
    return (EllipticCurvePoint) {x, y, false};
}

void testCrypto(Allocator* allocator) {
    EllipticCurve ec = { .a = 2, .b = 2, .p = 17 };
    EllipticCurvePoint p1 = {5, 1, false};
    EllipticCurvePoint pn = ecAdd(ec, p1, p1);
    expect("Elliptic Curve double [x]", pn.x, 6);
    expect("Elliptic Curve double [y]", pn.y, 3);
    EllipticCurvePoint pl = pn;
    for (int i=3; i<=21; i++) {
        pn = ecAdd(ec, p1, pl);
        if (pn.is_infinity) {
            printf("Elliptic Curve %dP: infinity\n", i);
        } else {
            printf("Elliptic Curve %dP: (%d, %d)\n", i, pn.x, pn.y);
        }
        pl = pn;
    }
}