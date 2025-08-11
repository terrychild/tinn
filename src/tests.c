#include <stdlib.h>
#include <stdio.h>

#include "lib.h"

int runTests() {
    // type tests
    expect("U64 size", sizeof(U64), 8);
    expect("U8 range", (U8)(-1), 255);
    expect("size_t is 64 bit", sizeof(size_t), sizeof(U64));

    // mem tests
    ArenaAllocator arena;
    arenaInit(&arena, 1);
    expect("arena page size", arena.size, 4096);
    arenaFree(&arena);

    arenaInit(&arena, 0);
    expect("default arena size", arena.size, GB(1));
    U8* data = arenaAllocZero(&arena, 12);
    expect("arena commit size", arena.committed, KB(8));
    expect("arena allocate size", arena.allocated, 16);
    expect("arena zero data", data[11], 0);
    data[11] = 14;    
    expect("arena data", data[11], 14);
    arenaFree(&arena);

    // report
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}