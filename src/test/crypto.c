#include "test/expect.h"
#include "lib/types.h"
#include "lib/maths/bigint.h"
#include "lib/crypto/ec.h"
#include "lib/crypto/mc.h"
#include "lib/crypto/x25519.h"
#include "lib/crypto/sha.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/cli.h"
#include "lib/bytes.h"

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
    point = ecPoint(mem, ec, biFrom(mem, 2));
    expect("Elliptic Curve secp256r1 2G x", point->x, biFrom(mem, "0x7cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978"));
    expect("Elliptic Curve secp256r1 2G y", point->y, biFrom(mem, "0x7775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1"));

    point = ecPoint(mem, ec, biFrom(mem, 10));
    expect("Elliptic Curve secp256r1 10G x", point->x, biFrom(mem, "0xcef66d6b2a3a993e591214d1ea223fb545ca6c471c48306e4c36069404c5723f"));
    expect("Elliptic Curve secp256r1 10G y", point->y, biFrom(mem, "0x878662a229aaae906e123cdd9d3b4c10590ded29fe751eeeca34bbaa44af0773"));


    MontgomeryCurve* mc = mcNew(mem, "0x76d06", "1", "0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed", "9");

    expect("Elliptic Curve 25519 2G", mcPublicKey(mem, mc, biFrom(mem, 2)), biFrom(mem, "0x20d342d51873f1b7d9750c687d1571148f3f5ced1e350b5c5cae469cdd684efb"));
    expect("Elliptic Curve 25519 3G", mcPublicKey(mem, mc, biFrom(mem, 3)), biFrom(mem, "0x1c12bc1a6d57abe645534d91c21bba64f8824e67621c0859c00a03affb713c12"));
    expect("Elliptic Curve 25519 10G", mcPublicKey(mem, mc, biFrom(mem, 10)), biFrom(mem, "0x41eda655b159060471fb4ce5d7cb3fe43ee51843d2080e0383ce42892c3a9c7b"));
    expect("Elliptic Curve 25519 256G", mcPublicKey(mem, mc, biFrom(mem, 256)), biFrom(mem, "0x4300017536976a742ec8747f7505cd6bc80e610d669acab1a1eed36f680d98e8"));

    expect("Elliptic Curve 25519 0x10000000", mcPublicKey(mem, mc, biFrom(mem, "0x10000000")), biFrom(mem, "0x6893cedbcf053a23b7d5676bf577dbc309d25c8525a0f2f5036b67f0967fa084"));
    expect("Elliptic Curve 25519 0xFFFFFFFF", mcPublicKey(mem, mc, biFrom(mem, "0xFFFFFFFF")), biFrom(mem, "0x08de23ae492b2c0a8b7c7e1f216501ba25bea603a30f5fd0c4bbac77fd5fec82"));
    expect("Elliptic Curve 25519 0x100000000", mcPublicKey(mem, mc, biFrom(mem, "0x100000000")), biFrom(mem, "0x06a8e0bfab511d23d72ddc7e8b0d4e3561183208d3d7886c269a92ca1de7dc30"));
    expect("Elliptic Curve 25519 0x1000000000000000", mcPublicKey(mem, mc, biFrom(mem, "0x1000000000000000")), biFrom(mem, "0x733350c17acebbbafc697ea8b5f03b354bf7ae172f54e86e883b59d150d1b370"));
    BigInt* x = mcPublicKey(mem, mc, biFrom(mem, "0x6A2CB91DA5FB77B12A99C0EB872F4CDF4566B25172C1163C7DA518730A6D0770"));
    expect("Elliptic Curve 25519", x, biFrom(mem, "0x6a4e9baa8ea9a4ebf41a38260d3abf0d5af73eb4dc7d8b7454a7308909f02085"));
    x = mcPublicKey(mem, mc, biFrom(mem, "0x4242424242424242424242424242424242424242424242424242424242424240"));
    expect("Elliptic Curve 25519", x, biFrom(mem, "0x72f493999c4dd1279321ae3a50c1cc1fe776aa283360727ed5fb10e02b442c13"));

    expect("25519 256G", x25519PublicKey(mem, biFrom(mem, 256)), biFrom(mem, "0x159ea268edbc1851e82e3d3910e5e866ca73a5d009816d98852abf80944a5507"));
    x = x25519PublicKey(mem, biFrom(mem, "0x4242424242424242424242424242424242424242424242424242424242424240"));
    expect("25519", x, biFrom(mem, "0x72f493999c4dd1279321ae3a50c1cc1fe776aa283360727ed5fb10e02b442c13"));

    U8 server_private_bytes[] = {0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf};
    U8 server_public_bytes[] = {0x9f, 0xd7, 0xad, 0x6d, 0xcf, 0xf4, 0x29, 0x8d, 0xd3, 0xf9, 0x6d, 0x5b, 0x1b, 0x2a, 0xf9, 0x10, 0xa0, 0x53, 0x5b, 0x14, 0x88, 0xd7, 0xf8, 0xfa, 0xbb, 0x34, 0x9a, 0x98, 0x28, 0x80, 0xb6, 0x15};
    BigInt* server_private = biFromLE(mem, server_private_bytes, 32);
    BigInt* server_public = x25519PublicKey(mem, server_private);
    expect("server public key", server_public, biFromLE(mem, server_public_bytes, 32));

    U8 client_public_bytes[] = {0x35, 0x80, 0x72, 0xd6, 0x36, 0x58, 0x80, 0xd1, 0xae, 0xea, 0x32, 0x9a, 0xdf, 0x91, 0x21, 0x38, 0x38, 0x51, 0xed, 0x21, 0xa2, 0x8e, 0x3b, 0x75, 0xe9, 0x65, 0xd0, 0xd2, 0xcd, 0x16, 0x62, 0x54};
    BigInt* client_public = biFromLE(mem, client_public_bytes, 32);

    U8 shrared_secret_bytes[] = {0xdf, 0x4a, 0x29, 0x1b, 0xaa, 0x1e, 0xb7, 0xcf, 0xa6, 0x93, 0x4b, 0x29, 0xb4, 0x74, 0xba, 0xad, 0x26, 0x97, 0xe2, 0x9f, 0x1f, 0x92, 0x0d, 0xcc, 0x77, 0xc8, 0xa0, 0xa0, 0x88, 0x44, 0x76, 0x24};
    expect("shared secret", x25519(mem, server_private, client_public), biFromLE(mem, shrared_secret_bytes, 32));

    Slice hash = sha1(mem, sliceFromStr("The quick brown fox jumps over the lazy dog"));
    expect("SHA-1, lazy dog", toHexStr(mem, hash), "0x2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
    hash = sha1(mem, sliceFromStr("The quick brown fox jumps over the lazy cog"));
    expect("SHA-1, lazy cog", toHexStr(mem, hash), "0xde9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3");
    hash = sha1(mem, sliceFromStr(""));
    expect("SHA-1, empty string", toHexStr(mem, hash), "0xda39a3ee5e6b4b0d3255bfef95601890afd80709");

    hash = sha224(mem, sliceFromStr("The quick brown fox jumps over the lazy dog"));
    expect("SHA-224, lazy dog", toHexStr(mem, hash), "0x730e109bd7a8a32b1cb9d9a09aa2325d2430587ddbc0c38bad911525");
    hash = sha224(mem, sliceFromStr("The quick brown fox jumps over the lazy dog."));
    expect("SHA-224, lazy dog.", toHexStr(mem, hash), "0x619cba8e8e05826e9b8c519c0a5c68f4fb653e8a3d8aa04bb2c8cd4c");
    hash = sha224(mem, sliceFromStr(""));
    expect("SHA-224, empty string", toHexStr(mem, hash), "0xd14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f");

    hash = sha256(mem, sliceFromStr("abc"));
    expect("SHA-256, abc", toHexStr(mem, hash), "0xba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    hash = sha256(mem, sliceFromStr(""));
    expect("SHA-256, empty string", toHexStr(mem, hash), "0xe3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    memoryDebug(mem);
    scratchRelease(scratch);
}