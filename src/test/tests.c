#include <stdlib.h>

#include "test/expect.h"
#include "lib/lib.h"

int runTests() {
    expect_failed = false;

    PRINT(CC_BLUE, "================\n Type tests\n================\n");

    expect("U8 size", sizeof(U8), 1);
    expect("U64 size", sizeof(U64), 8);
    expect("U8 range", (U8)(-1), 255);
    expect("size_t size", sizeof(size_t), sizeof(U64));


    PRINT(CC_BLUE, "================\n Log tests\n================\n");

    logOpen("./test.log");
    DEBUG("Debug");
    LOG("A log...no not one of those.");
    WARN("A warning.");
    ERROR("An error.");
    //PANIC("PANIC!!!");
    logClose();


    PRINT(CC_BLUE, "================\n Arena tests\n================\n");

    Arena arena;
    arenaInit(&arena, 1);
    expect("page size", arena.size, KB(4));
    expect("committed", arena.committed, 0);
    expect("allocated", arena.allocated, 0);
    arenaRelease(&arena);

    arenaInit(&arena, 0);
    expect("default size", arena.size, GB(1));
    U8* data = arenaAlloc(&arena, 12);
    expect("committed", arena.committed, KB(4));
    expect("allocated", arena.allocated, 16);
    expect("zero data", data[11], 0);
    data[11] = 14;    
    expect("data", data[11], 14);
    arenaReset(&arena);
    expect("data after reset", data[11], 14);
    data = arenaAllocRaw(&arena, 12);
    expect("data after raw", data[11], 14);
    arenaReset(&arena);
    data = arenaAlloc(&arena, 12);
    expect("data after reset and alloc", data[11], 0);

    arenaPushFrame(&arena);
    expect("allocated after push frame", arena.allocated, 24);
    arenaAlloc(&arena, 1);
    expect("allocated after alloc", arena.allocated, 32);
    arenaPushFrame(&arena);
    expect("allocated after second push frame", arena.allocated, 40);
    arenaPopFrame(&arena);
    expect("allocated after pop", arena.allocated, 32);
    arenaPopFrame(&arena);
    expect("allocated after second pop", arena.allocated, 16);
    arenaPopFrame(&arena);
    expect("allocated after third pop", arena.allocated, 0);
    arenaPopFrame(&arena);
    expect("allocated after fourth pop", arena.allocated, 0);
    arenaPushFrame(&arena);
    expect("allocated after push", arena.allocated, 8);
    arenaPopFrame(&arena);
    expect("allocated after pop", arena.allocated, 0);

    arenaRelease(&arena);


    PRINT(CC_BLUE, "================\n Report\n================\n");
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}