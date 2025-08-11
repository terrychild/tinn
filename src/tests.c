#include <stdlib.h>
#include <stdio.h>

#include "lib.h"

int runTests() {
    // type tests
    expect("u64 size", sizeof(u64), 8);
    expect("u8 range", (u8)(-1), 255);
    expect("size_t is 64 bit", sizeof(size_t), sizeof(u64));

    // mem tests
    ArenaAllocator arena;
    arenaInit(&arena, 1);
    expect("arena page size", arena.size, 4096);
    arenaFree(&arena);

    arenaInit(&arena, 0);
    expect("default arena size", arena.size, GB(1));
    u8* data = arenaAllocZero(&arena, 12);
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