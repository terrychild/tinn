#include "test/expect.h"

#include "lib/types.h"
#include "lib/maths/bigint.h"
#include "lib/mem/allocator.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/cli.h"

void testMaths(Allocator* allocator) {
    BigInt* bi = biFromInt(allocator, 14);

    expect("bi 15", biAdd(bi, 1), biFromInt(allocator, 15));
    expect("bi 30", biAdd(bi, bi), biFromInt(allocator, 30));
    expect("bi 29", biSub(bi, 1), biFromInt(allocator, 29));

    expect("cmp 29", biCmp(bi, 29), 0);
    expect("cmp 29", biCmp(bi, 30), -1);
    expect("cmp 29", biCmp(bi, 28), 1);

    biSetHex(bi, "FFFFFFFFFFFFFFFF");
    expect("bi FF* (size)", bi->size, 1);
    biAdd(bi, 1);
    expect("bi FF* + 1 (size)", bi->size, 2);
    expect("bi FF* + 1 (value)", bi, biFromHex(allocator, "10000000000000000"));

    expect("shift right", biShiftRight(bi, 8), biFromHex(allocator, "100000000000000"));
    expect("shift left", biShiftLeft(bi, 8), biFromHex(allocator, "10000000000000000"));
    expect("shift left", biShiftLeft(bi, 1), biFromHex(allocator, "20000000000000000"));

    expect("*8", biMul(biSetInt(bi, 8), 8), biFromInt(allocator, 64));
    expect("square", biMul(bi, bi), biFromInt(allocator, 4096));
    expect("square", biMul(bi, bi), biFromInt(allocator, 16777216));
    expect("square", biMul(bi, bi), biFromInt(allocator, 281474976710656));
    expect("square", biMul(bi, bi), biFromHex(allocator, "1000000000000000000000000"));

    BigInt* remainder = biNew(allocator);
    expect("/3", biDivRem(bi, 3, remainder), biFromHex(allocator, "555555555555555555555555"));
    expect("remainder", remainder, biFromInt(allocator, 1));
    expect("mod 1024", biMod(bi, 1024), biFromInt(allocator, 341));

    expect("/4", biDiv(bi, 4), biFromInt(allocator, 85));

    expect("2 pow 16", biPow(biSetInt(bi, 2), 16), biFromInt(allocator, 65536));
    expect("2 pow 16 pow 16", biPow(bi, 16), biFromHex(allocator, "10000000000000000000000000000000000000000000000000000000000000000"));

    BigInt* bi_MAX = biFromHex(allocator, "FFFFFFFFFFFFFFFF");
    BigInt* bi_2MAX = biFromHex(allocator, "1FFFFFFFFFFFFFFFE");
    expect("big first", biMul(biSet(bi, bi_MAX), 2), bi_2MAX);
    expect("big second", biMul(biSetInt(bi, 2), bi_MAX), bi_2MAX);

    expect("281474976710656/37", biDivRem(biSetInt(bi, 281474976710656), 37, remainder), biFromInt(allocator, 7607431802990));
    PRINT(CC_BLUE, "          %lu\n", bi->limbs[0]);
    expect("remainder", remainder, biFromInt(allocator, 26));
    PRINT(CC_BLUE, "          %lu\n", remainder->limbs[0]);

    allocatorDebug(allocator);
    allocatorReset(allocator);
}