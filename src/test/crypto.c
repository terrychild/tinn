#include "test/expect.h"
#include "lib/types.h"
#include "lib/maths/bigint.h"
#include "lib/crypto/ec.h"
#include "lib/crypto/mc.h"
#include "lib/crypto/curve25519.h"
#include "lib/mem/memory.h"
#include "lib/cli.h"

void testCrypto() {
    PRINT(CC_BLUE, "================\n Crypto tests\n================\n");
    ScratchMemory scratch = scratchMemory(NULL);
    Memory* mem = scratch.mem;

    EllipticCurve* ec = ecNew(mem, "2", "2", "17", "5", "1");
    EllipticCurvePoint* point = ecPoint(mem, ec, biFromInt(mem, 2));
    expect("Elliptic Curve double [x]", point->x->limbs[0], 6);
    expect("Elliptic Curve double [y]", point->y->limbs[0], 3);
    for (int i=3; i<=21; i++) {
        point = ecPoint(mem, ec, biFromInt(mem, i));
        if (point->is_infinity) {
            PRINT(CC_YELLOW, "Elliptic Curve %dP: infinity\n", i);
        } else {
            PRINT(CC_YELLOW, "Elliptic Curve %dP: (%lu, %lu)\n", i, point->x->limbs[0], point->y->limbs[0]);
        }
    }

    ec = ecNew(mem,
        "0xffffffff00000001000000000000000000000000fffffffffffffffffffffffc",
        "0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b",
        "0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff",
        "0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",
        "0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5"
    );
    point = ecPoint(mem, ec, biFromInt(mem, 2));
    expect("Elliptic Curve secp256r1 2G x", point->x, biFrom(mem, "0x7cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978"));
    expect("Elliptic Curve secp256r1 2G y", point->y, biFrom(mem, "0x7775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1"));

    point = ecPoint(mem, ec, biFromInt(mem, 10));
    expect("Elliptic Curve secp256r1 10G x", point->x, biFrom(mem, "0xcef66d6b2a3a993e591214d1ea223fb545ca6c471c48306e4c36069404c5723f"));
    expect("Elliptic Curve secp256r1 10G y", point->y, biFrom(mem, "0x878662a229aaae906e123cdd9d3b4c10590ded29fe751eeeca34bbaa44af0773"));


    MontgomeryCurve* mc = mcNew(mem, "0x76d06", "1", "0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed", "9");

    expect("Elliptic Curve 25519 2G", mcKey(mem, mc, biFromInt(mem, 2)), biFrom(mem, "0x20d342d51873f1b7d9750c687d1571148f3f5ced1e350b5c5cae469cdd684efb"));
    expect("Elliptic Curve 25519 3G", mcKey(mem, mc, biFromInt(mem, 3)), biFrom(mem, "0x1c12bc1a6d57abe645534d91c21bba64f8824e67621c0859c00a03affb713c12"));
    expect("Elliptic Curve 25519 10G", mcKey(mem, mc, biFromInt(mem, 10)), biFrom(mem, "0x41eda655b159060471fb4ce5d7cb3fe43ee51843d2080e0383ce42892c3a9c7b"));
    expect("Elliptic Curve 25519 256G", mcKey(mem, mc, biFromInt(mem, 256)), biFrom(mem, "0x4300017536976a742ec8747f7505cd6bc80e610d669acab1a1eed36f680d98e8"));

    expect("Elliptic Curve 25519 0x10000000", mcKey(mem, mc, biFrom(mem, "0x10000000")), biFrom(mem, "0x6893cedbcf053a23b7d5676bf577dbc309d25c8525a0f2f5036b67f0967fa084"));
    expect("Elliptic Curve 25519 0xFFFFFFFF", mcKey(mem, mc, biFrom(mem, "0xFFFFFFFF")), biFrom(mem, "0x08de23ae492b2c0a8b7c7e1f216501ba25bea603a30f5fd0c4bbac77fd5fec82"));
    expect("Elliptic Curve 25519 0x100000000", mcKey(mem, mc, biFrom(mem, "0x100000000")), biFrom(mem, "0x06a8e0bfab511d23d72ddc7e8b0d4e3561183208d3d7886c269a92ca1de7dc30"));
    expect("Elliptic Curve 25519 0x1000000000000000", mcKey(mem, mc, biFrom(mem, "0x1000000000000000")), biFrom(mem, "0x733350c17acebbbafc697ea8b5f03b354bf7ae172f54e86e883b59d150d1b370"));
    BigInt* x = mcKey(mem, mc, biFrom(mem, "0x6A2CB91DA5FB77B12A99C0EB872F4CDF4566B25172C1163C7DA518730A6D0770"));
    expect("Elliptic Curve 25519", x, biFrom(mem, "0x6a4e9baa8ea9a4ebf41a38260d3abf0d5af73eb4dc7d8b7454a7308909f02085"));
    x = mcKey(mem, mc, biFrom(mem, "0x4242424242424242424242424242424242424242424242424242424242424240"));
    expect("Elliptic Curve 25519", x, biFrom(mem, "0x72f493999c4dd1279321ae3a50c1cc1fe776aa283360727ed5fb10e02b442c13"));

    expect("25519 256G", curve25519Key(mem, 256), biFrom(mem, "0x4300017536976a742ec8747f7505cd6bc80e610d669acab1a1eed36f680d98e8"));
    x = curve25519Key(mem, "4242424242424242424242424242424242424242424242424242424242424240");
    expect("25519", x, biFrom(mem, "0x72f493999c4dd1279321ae3a50c1cc1fe776aa283360727ed5fb10e02b442c13"));

    BigInt* server_private = biFromInt(mem, 7);
    BigInt* server_public = curve25519Key(mem, server_private);
    expect("server public key", server_public, biFrom(mem, "0x0daf32e7ed8099122b2dfa4c1d8c4a20c0972a1538bf0575338aae0fe0841828"));

    BigInt* client_private = biFromInt(mem, 5);
    BigInt* client_public = curve25519Key(mem, client_private);
    expect("client public key", client_public, biFrom(mem, "0x41b6ec3c50ee7af203c0026e5e079e7fa8cbc9bc581d49cb0d537d5778497c87"));

    expect("server shared secret", curve25519Shared(mem, client_public, server_private), biFrom(mem, "0x1b9b715c415e547a13ca98a9f561d54499cc7402a1098414eddcfaf83ffbe3f8"));
    expect("client shared secret", curve25519Shared(mem, server_public, client_private), biFrom(mem, "0x1b9b715c415e547a13ca98a9f561d54499cc7402a1098414eddcfaf83ffbe3f8"));

    server_private = biFrom(mem, "0x1fd7ad6dcff4298dd3f96d5b1b2af910a0535b1488d7f8fabb349a982880b615");
    server_public = curve25519Key(mem, server_private);
    expect("server public key", server_public, biFrom(mem, "0x46992c04126de9d9cafce4add60c45f920f4acf094529e5b76abe9befb67da06"));

    server_private = biFrom(mem, "0xafaeadacabaaa9a8a7a6a5a4a3a2a1a09f9e9d9c9b9a99989796959493929190");
    server_public = curve25519Key(mem, server_private);
    //expect("server public key", server_public, biFrom(mem, "0x15b68028989a34bbfaf8d788145b53a010f92a1b5b6df9d38d29f4cf6dadd79f"));
    expect("server public key", server_public, biFrom(mem, "0x15b68028989a34bbfaf8d788145b53a010f92a1b5b6df9d38d29f4cf6dadd79f"));

    memoryDebug(mem);
    scratchRelease(scratch);
}