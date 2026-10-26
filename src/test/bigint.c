#include "test/expect.h"

#include "lib/types.h"
#include "lib/mem/allocator.h"
#include "lib/bigint.h"
#include "lib/bytes.h"
#include "lib/cli.h"

void testBigInt(Allocator* allocator) {
    PRINT(CC_BLUE, "max\n");
    BigInt* max = biFromInt(allocator, 9223372036854775807);
    hexDump(biToBESlice(max));

    PRINT(CC_BLUE, "one\n");
    BigInt* one = biFromInt(allocator, 1);
    hexDump(biToBESlice(one));

    PRINT(CC_BLUE, "max cloned\n");
    BigInt* bi = biClone(allocator, max);
    hexDump(biToBESlice(bi));

    PRINT(CC_BLUE, "add one\n");
    biAdd(bi, bi, one);
    hexDump(biToBESlice(bi));

    PRINT(CC_BLUE, "add slef\n");
    biAdd(bi, bi, bi);
    hexDump(biToBESlice(bi));

    PRINT(CC_BLUE, "sub one\n");
    biSub(bi, bi, one);
    hexDump(biToBESlice(bi));

    PRINT(CC_BLUE, "shift left 64\n");
    biShiftLeft(bi, bi, 64);
    hexDump(biToBESlice(bi));
    PRINT(CC_BLUE, "shift left 1\n");
    biShiftLeft(bi, bi, 1);
    hexDump(biToBESlice(bi));

    PRINT(CC_BLUE, "shift right 4\n");
    biShiftRight(bi, bi, 4);
    hexDump(biToBESlice(bi));
    PRINT(CC_BLUE, "shift right 1\n");
    biShiftRight(bi, bi, 1);
    hexDump(biToBESlice(bi));
    PRINT(CC_BLUE, "shift right 1\n");
    biShiftRight(bi, bi, 1);
    hexDump(biToBESlice(bi));

    PRINT(CC_BLUE, "set 2\n");
    biSetInt(bi, 2);
    hexDump(biToBESlice(bi));

    for (int i=0; i<10; i++) {
        PRINT(CC_BLUE, "times %d\n", i);
        biMul(bi, bi, bi);
        hexDump(biToBESlice(bi));
    }

    PRINT(CC_BLUE, "divide by 2\n");
    BigInt* two = biFromInt(allocator, 2);
    BigInt* remainder = biNew(allocator);
    biDiv(bi, bi, two, remainder);
    hexDump(biToBESlice(bi));
    PRINT(CC_BLUE, "remainder\n");
    hexDump(biToBESlice(remainder));

    PRINT(CC_BLUE, "divide by 2\n");
    biDiv(bi, bi, two, remainder);
    hexDump(biToBESlice(bi));
    PRINT(CC_BLUE, "remainder\n");
    hexDump(biToBESlice(remainder));

    PRINT(CC_BLUE, "divide by 3\n");
    biDiv(bi, bi, biFromInt(allocator, 3), remainder);
    hexDump(biToBESlice(bi));
    PRINT(CC_BLUE, "remainder\n");
    hexDump(biToBESlice(remainder));

    PRINT(CC_BLUE, "mod by 9\n");
    biMod(remainder, bi, biFromInt(allocator, 9));
    hexDump(biToBESlice(remainder));

    for (int i=1; i<10; i++) {
        PRINT(CC_BLUE, "10 to power %d\n", i);
        biPow(bi, biFromInt(allocator, 10), biFromInt(allocator, i));
        hexDump(biToBESlice(bi));
    }

    PRINT(CC_BLUE, "set to 0x6A\n");
    biSetInt(bi, 0x6A);
    hexDump(biToBESlice(bi));

    for (int i=0; i<8; i++) {
        PRINT(CC_BLUE, "bit %d: %d\n", i, biBit(bi, i));
    }

    allocatorDebug(allocator);
    allocatorReset(allocator);
}