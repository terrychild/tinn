#include "test/expect.h"

#include "lib/types.h"
#include "lib/maths/mpa.h"
#include "lib/maths/bigint.h"
#include "lib/mem/allocator.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/cli.h"

void testMaths(Allocator* allocator) {
    U64 key[4];
    expect("set int to 256 (bit length)", mpaSetInt(key, 4, 256), 9);
    U8 bytes_256[] = {1, 0};
    expect("set int to 256", mpaToBESlice(allocator, key, 4), sliceNew(bytes_256, 2));

    expect("set int to U64 max (bit length)", mpaSetHex(key, 4, "FFFFFFFFFFFFFFFF"), 64);
    U8 bytes_u64_max[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    expect("set int to U64 max", mpaToBESlice(allocator, key, 4), sliceNew(bytes_u64_max, 8));
    expect("cmp U64 max", mpaCmpInt(key, 0xFFFFFFFFFFFFFFFF, 4), 0);

    expect("set int to hex 42* (bit length)", mpaSetHex(key, 4, "4242424242424242424242424242424242424242424242424242424242424242"), 255);
    U8 bytes_42s[] = {
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42
    };
    expect("set int to hex 42*", mpaToBESlice(allocator, key, 4), sliceNew(bytes_42s, 32));

    U64 key_plus[4];
    mpaSetHex(key_plus, 4, "4242424242424242424242424242424242424242424242424242424242424243");
    U64 key_minus[4];
    mpaSetHex(key_minus, 4, "4242424242424242424242424242424242424242424242424242424242424241");
    expect("cmp plus", mpaCmp(key, key_plus, 4), -1);
    expect("cmp minus", mpaCmp(key, key_minus, 4), 1);

    U64 one[4];
    mpaSetInt(one, 4, 1);
    expect("add one (carry)", mpaAdd(key, one, 4), 0);
    U8 bytes_42s_plus[] = {
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x43
    };
    expect("add one (value)", mpaToBESlice(allocator, key, 4), sliceNew(bytes_42s_plus, 32));
    expect("add one (cmp)", mpaCmp(key, key_plus, 4), 0);

    U64 two[4];
    mpaSetInt(two, 4, 2);
    expect("minus two (borrow)", mpaSub(key, two, 4), 0);
    U8 bytes_42s_minus[] = {
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x41
    };
    expect("minus two (value)", mpaToBESlice(allocator, key, 4), sliceNew(bytes_42s_minus, 32));
    expect("minus two (cmp)", mpaCmp(key, key_minus, 4), 0);

    mpaSetHex(key, 4, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    expect("carry", mpaAdd(key, one, 4), 1);
    expect("is zero", mpaIsZero(key, 4), true);
    expect("borrow", mpaSub(key, one, 4), 1);

    U8 bytes_max[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    expect("max", mpaToBESlice(allocator, key, 4), sliceNew(bytes_max, 32));

    BigInt* bi = biFromInt(allocator, 14);
    BigInt* bi_1 = biFromInt(allocator, 1);
    BigInt* bi_15 = biFromInt(allocator, 15);
    BigInt* bi_30 = biFromInt(allocator, 30);
    BigInt* bi_29 = biFromInt(allocator, 29);

    expect("bi 15", biAdd(bi, bi_1), bi_15);
    expect("bi 30", biAdd(bi, bi), bi_30);
    expect("bi 29", biSub(bi, bi_1), bi_29);

    biSetHex(bi, "FFFFFFFFFFFFFFFF");
    expect("bi FF* (size)", bi->size, 1);
    biAdd(bi, bi_1);
    expect("bi FF* + 1 (size)", bi->size, 2);
    expect("bi FF* + 1 (value)", bi, biFromHex(allocator, "10000000000000000"));

    expect("shift right", biShiftRight(bi, 8), biFromHex(allocator, "100000000000000"));
    expect("shift left", biShiftLeft(bi, 8), biFromHex(allocator, "10000000000000000"));
    expect("shift left", biShiftLeft(bi, 1), biFromHex(allocator, "20000000000000000"));

    BigInt* bi_8 = biFromInt(allocator, 8);
    expect("*8", biMul(biCopy(bi, bi_8), bi_8), biFromInt(allocator, 64));
    expect("square", biMul(bi, bi), biFromInt(allocator, 4096));
    expect("square", biMul(bi, bi), biFromInt(allocator, 16777216));
    expect("square", biMul(bi, bi), biFromInt(allocator, 281474976710656));
    expect("square", biMul(bi, bi), biFromHex(allocator, "1000000000000000000000000"));

    BigInt* remainder = biNew(allocator);
    expect("/3", biDivRem(bi, biFromInt(allocator, 3), remainder), biFromHex(allocator, "555555555555555555555555"));
    expect("remainder", remainder, bi_1);
    expect("mod 1024", biMod(bi, biFromInt(allocator, 1024)), biFromInt(allocator, 341));

    expect("/4", biDiv(bi, biFromInt(allocator, 4)), biFromInt(allocator, 85));

    expect("2 pow 16", biPow(biSetInt(bi, 2), biFromInt(allocator, 16)), biFromInt(allocator, 65536));
    expect("2 pow 16 pow 16", biPow(bi, biFromInt(allocator, 16)), biFromHex(allocator, "10000000000000000000000000000000000000000000000000000000000000000"));

    BigInt* bi_2 = biFromInt(allocator, 2);
    BigInt* bi_MAX = biFromHex(allocator, "FFFFFFFFFFFFFFFF");
    BigInt* bi_2MAX = biFromHex(allocator, "1FFFFFFFFFFFFFFFE");
    expect("big first", biMul(biCopy(bi, bi_MAX), bi_2), bi_2MAX);
    expect("big second", biMul(biCopy(bi, bi_2), bi_MAX), bi_2MAX);

    expect("281474976710656/37", biDivRem(biSetInt(bi, 281474976710656), biFromInt(allocator, 37), remainder), biFromInt(allocator, 7607431802990));
    PRINT(CC_BLUE, "          %lu\n", bi->limbs[0]);
    expect("remainder", remainder, biFromInt(allocator, 26));
    PRINT(CC_BLUE, "          %lu\n", remainder->limbs[0]);

    allocatorDebug(allocator);
    allocatorReset(allocator);
}