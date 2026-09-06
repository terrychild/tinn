#include <stdlib.h>

#include "test/expect.h"
#include "lib.h"

int runTests() {
    expect_failed = false;

    // type tests
    expect("U64 size", sizeof(U64), 8);
    expect("U8 range", (U8)(-1), 255);
    expect("size_t is 64 bit", sizeof(size_t), sizeof(U64));

    // mem tests
    ArenaAllocator* arena = arenaNew(1);
    expect("arena page size", arena->size, KB(4));
    expect("arena committed", arena->committed, KB(4));
    expect("arena allocated", arena->allocated, sizeof(*arena));
    arenaRelease(arena);

    arena = arenaNew(0);
    expect("default arena size", arena->size, GB(1));
    U8* data = arenaAlloc(arena, 12);
    expect("arena commit size", arena->committed, KB(4));
    expect("arena allocate size", arena->allocated, sizeof(*arena) + 16);
    expect("arena zero data", data[11], 0);
    data[11] = 14;    
    expect("arena data", data[11], 14);
    arenaReset(arena);
    expect("arena data after reset", data[11], 14);
    data = arenaAllocRaw(arena, 12);
    expect("arena data after raw", data[11], 14);
    arenaReset(arena);
    data = arenaAlloc(arena, 12);
    expect("arena data after reset and alloc", data[11], 0);
    arenaRelease(arena);

    // report
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}