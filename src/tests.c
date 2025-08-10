#include <stdlib.h>
#include <stdio.h>

#include "lib.h"

int run_tests() {
    // type tests
    expect("u64 size", sizeof(u64), 8);
    expect("u8 range", (u8)(-1), 255);
    expect("size_t is 64 bit", sizeof(size_t), sizeof(u64));

    // mem tests
    ArenaAllocator arena;
    arenaInit(&arena, 100);
    expect("arena page size", arena.size, 4096);

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