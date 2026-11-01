#include <stdlib.h>

#include "test/expect.h"

#include "lib/types.h"
#include "lib/maths/bigint.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/cli.h"

void testMaths() {
    PRINT(CC_BLUE, "================\n Maths tests\n================\n");
    ScratchMemory scratch = scratchMemory(NULL);
    Memory* mem = scratch.mem;

    BigInt* bi = biFromInt(mem, 14);

    expect("bi 15", biAdd(bi, 1), biFromInt(mem, 15));
    expect("bi 30", biAdd(bi, bi), biFromInt(mem, 30));
    expect("bi 29", biSub(bi, 1), biFromInt(mem, 29));

    expect("cmp 29", biCmp(bi, 29), 0);
    expect("cmp 29", biCmp(bi, 30), -1);
    expect("cmp 29", biCmp(bi, 28), 1);

    biSetHex(bi, "FFFFFFFFFFFFFFFF");
    expect("bi FF* (size)", bi->size, 1);
    biAdd(bi, 1);
    expect("bi FF* + 1 (size)", bi->size, 2);
    expect("bi FF* + 1 (value)", bi, biFromHex(mem, "10000000000000000"));

    /*expect("shift right", biShiftRight(bi, 8), biFromHex(mem, "100000000000000"));
    expect("shift left", biShiftLeft(bi, 8), biFromHex(mem, "10000000000000000"));
    expect("shift left", biShiftLeft(bi, 1), biFromHex(mem, "20000000000000000"));*/

    expect("8*8", biMul(bi, 8, 8), biFromInt(mem, 64));
    expect("square", biMul(bi, bi), biFromInt(mem, 4096));
    expect("square", biMul(bi, bi), biFromInt(mem, 16777216));
    expect("square", biMul(bi, bi), biFromInt(mem, 281474976710656));
    expect("square", biMul(bi, bi), biFromHex(mem, "1000000000000000000000000"));

    BigInt* remainder = biNew(mem);
    expect("/3", biDiv(bi, bi, 3, remainder), biFromHex(mem, "555555555555555555555555"));
    expect("remainder", remainder, biFromInt(mem, 1));
    expect("mod 1024", biMod(bi, 1024), biFromInt(mem, 341));

    expect("/4", biDiv(bi, 4), biFromInt(mem, 85));

    expect("2 pow 16", biPow(bi, 2, 16, NULL), biFromInt(mem, 65536));
    expect("2 pow 16 pow 16", biPow(bi, 16), biFromHex(mem, "10000000000000000000000000000000000000000000000000000000000000000"));

    BigInt* bi_MAX = biFromHex(mem, "FFFFFFFFFFFFFFFF");
    BigInt* bi_2MAX = biFromHex(mem, "1FFFFFFFFFFFFFFFE");
    expect("big first", biMul(mem, bi_MAX, 2, NULL), bi_2MAX);
    expect("big second", biMul(mem, biSetInt(bi, 2), bi_MAX, NULL), bi_2MAX);

    expect("281474976710656/37", biDiv(bi, 281474976710656, 37, remainder), biFromInt(mem, 7607431802990));
    PRINT(CC_BLUE, "          %lu\n", bi->limbs[0]);
    expect("remainder", remainder, biFromInt(mem, 26));
    PRINT(CC_BLUE, "          %lu\n", remainder->limbs[0]);

    memoryDebug(mem);
    scratchRelease(scratch);
}