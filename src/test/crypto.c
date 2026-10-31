#include "test/expect.h"
#include "lib/types.h"
#include "lib/maths/bigint.h"
#include "lib/crypto/ec.h"
#include "lib/crypto/mc.h"
#include "lib/mem/allocator.h"
#include "lib/cli.h"

void testCrypto(Allocator* allocator) {
    EllipticCurve* ec = ecNew(allocator, "2", "2", "11", "5", "1");
    EllipticCurvePoint* point = ecNewPoint(allocator, ec);
    ecPoint(point, ec, biFromInt(allocator, 2));
    expect("Elliptic Curve double [x]", point->x->limbs[0], 6);
    expect("Elliptic Curve double [y]", point->y->limbs[0], 3);
    for (int i=3; i<=21; i++) {
        ecPoint(point, ec, biFromInt(allocator, i));
        if (point->is_infinity) {
            PRINT(CC_YELLOW, "Elliptic Curve %dP: infinity\n", i);
        } else {
            PRINT(CC_YELLOW, "Elliptic Curve %dP: (%lu, %lu)\n", i, point->x->limbs[0], point->y->limbs[0]);
        }
    }

    ec = ecNew(allocator,
        "ffffffff00000001000000000000000000000000fffffffffffffffffffffffc",
        "5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b",
        "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff",
        "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",
        "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5"
    );
    ecPoint(point, ec, biFromInt(allocator, 2));
    expect("Elliptic Curve secp256r1 2G x", point->x, biFromHex(allocator, "7cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978"));
    expect("Elliptic Curve secp256r1 2G y", point->y, biFromHex(allocator, "7775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1"));

    ecPoint(point, ec, biFromInt(allocator, 10));
    expect("Elliptic Curve secp256r1 10G x", point->x, biFromHex(allocator, "cef66d6b2a3a993e591214d1ea223fb545ca6c471c48306e4c36069404c5723f"));
    expect("Elliptic Curve secp256r1 10G y", point->y, biFromHex(allocator, "878662a229aaae906e123cdd9d3b4c10590ded29fe751eeeca34bbaa44af0773"));


    MontgomeryCurve* mc = mcNew(allocator, "76d06", "1", "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed", "9");

    BigInt* x = biNew(allocator);
    mcKey(x, mc, biFromInt(allocator, 2));
    expect("Elliptic Curve 25519 2G", x, biFromHex(allocator, "20d342d51873f1b7d9750c687d1571148f3f5ced1e350b5c5cae469cdd684efb"));
    mcKey(x, mc, biFromInt(allocator, 3));
    expect("Elliptic Curve 25519 3G", x, biFromHex(allocator, "1c12bc1a6d57abe645534d91c21bba64f8824e67621c0859c00a03affb713c12"));
    mcKey(x, mc, biFromInt(allocator, 10));
    expect("Elliptic Curve 25519 10G", x, biFromHex(allocator, "41eda655b159060471fb4ce5d7cb3fe43ee51843d2080e0383ce42892c3a9c7b"));
    mcKey(x, mc, biFromInt(allocator, 256));
    expect("Elliptic Curve 25519 256G", x, biFromHex(allocator, "4300017536976a742ec8747f7505cd6bc80e610d669acab1a1eed36f680d98e8"));

    mcKey(x, mc, biFromHex(allocator, "10000000"));
    expect("Elliptic Curve 25519 0x10000000", x, biFromHex(allocator, "6893cedbcf053a23b7d5676bf577dbc309d25c8525a0f2f5036b67f0967fa084"));
    mcKey(x, mc, biFromHex(allocator, "FFFFFFFF"));
    expect("Elliptic Curve 25519 0xFFFFFFFF", x, biFromHex(allocator, "08de23ae492b2c0a8b7c7e1f216501ba25bea603a30f5fd0c4bbac77fd5fec82"));
    mcKey(x, mc, biFromHex(allocator, "100000000"));
    expect("Elliptic Curve 25519 0x100000000", x, biFromHex(allocator, "06a8e0bfab511d23d72ddc7e8b0d4e3561183208d3d7886c269a92ca1de7dc30"));
    mcKey(x, mc, biFromHex(allocator, "1000000000000000"));
    expect("Elliptic Curve 25519 0x1000000000000000", x, biFromHex(allocator, "733350c17acebbbafc697ea8b5f03b354bf7ae172f54e86e883b59d150d1b370"));
    mcKey(x, mc, biFromHex(allocator, "6A2CB91DA5FB77B12A99C0EB872F4CDF4566B25172C1163C7DA518730A6D0770"));
    expect("Elliptic Curve 25519", x, biFromHex(allocator, "6a4e9baa8ea9a4ebf41a38260d3abf0d5af73eb4dc7d8b7454a7308909f02085"));
    mcKey(x, mc, biFromHex(allocator, "4242424242424242424242424242424242424242424242424242424242424240"));
    expect("Elliptic Curve 25519", x, biFromHex(allocator, "72f493999c4dd1279321ae3a50c1cc1fe776aa283360727ed5fb10e02b442c13"));

    allocatorDebug(allocator);
    allocatorReset(allocator);
}