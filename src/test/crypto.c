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

EllipticCurvePoint ecDouble(EllipticCurve e, EllipticCurvePoint p1) {
    U64 s = mod(((3 * p1.x * p1.x) + e.a) * inverse(2 * p1.y, e.p), e.p);
    EllipticCurvePoint p2;
    p2.x = mod((s * s) - p1.x - p1.x, e.p);
    p2.y = mod((s * (p1.x - p2.x)) - p1.y, e.p);
    return p2;
}

void testCrypto(Allocator* allocator) {
    EllipticCurve ec = { .a = 2, .b = 2, .p = 17 };
    EllipticCurvePoint p1 = { .x = 5, .y = 1};
    EllipticCurvePoint p2 = ecDouble(ec, p1);
    expect("Elliptic Curve double [x]", p2.x, 6);
    expect("Elliptic Curve double [y]", p2.y, 3);
}