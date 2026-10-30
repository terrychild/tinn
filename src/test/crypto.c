#include "test/expect.h"
#include "lib/types.h"
#include "lib/maths/bigint.h"
#include "lib/crypto/ec.h"
#include "lib/mem/allocator.h"
#include "lib/cli.h"

void testCrypto(Allocator* allocator) {
    EllipticCurve* ec = ecNew(allocator, "2", "2", "11", "5", "1");
    EllipticCurvePoint* pn = ecPoint(allocator, ec, biFromInt(allocator, 2));
    expect("Elliptic Curve double [x]", pn->x->limbs[0], 6);
    expect("Elliptic Curve double [y]", pn->y->limbs[0], 3);
    for (int i=3; i<=21; i++) {
        pn = ecPoint(allocator, ec, biFromInt(allocator, i));
        if (pn->is_infinity) {
            PRINT(CC_YELLOW, "Elliptic Curve %dP: infinity\n", i);
        } else {
            PRINT(CC_YELLOW, "Elliptic Curve %dP: (%lu, %lu)\n", i, pn->x->limbs[0], pn->y->limbs[0]);
        }
    }

    ec = ecNew(allocator,
        "ffffffff00000001000000000000000000000000fffffffffffffffffffffffc",
        "5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b",
        "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff",
        "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",
        "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5"
    );
    pn = ecPoint(allocator, ec, biFromInt(allocator, 2));
    expect("Elliptic Curve secp256r1 2G x", pn->x, biFromHex(allocator, "7cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978"));
    expect("Elliptic Curve secp256r1 2G y", pn->y, biFromHex(allocator, "7775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1"));

    pn = ecPoint(allocator, ec, biFromInt(allocator, 10));
    expect("Elliptic Curve secp256r1 10G x", pn->x, biFromHex(allocator, "cef66d6b2a3a993e591214d1ea223fb545ca6c471c48306e4c36069404c5723f"));
    expect("Elliptic Curve secp256r1 10G y", pn->y, biFromHex(allocator, "878662a229aaae906e123cdd9d3b4c10590ded29fe751eeeca34bbaa44af0773"));

    allocatorDebug(allocator);
    allocatorReset(allocator);
}