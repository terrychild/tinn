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

    PRINT(CC_BLUE, "simple arena\n");
    Arena arena;
    arenaInit(&arena, 1, false);
    expect("page size", arena.size, KB(4));
    expect("committed", arena.committed, 0);
    expect("allocated", arena.allocated, 0);
    U8* data = arenaAlloc(&arena, 12);
    expect("committed after alloc", arena.committed, KB(4));
    expect("allocated after alloc", arena.allocated, 16);
    expect("zero data", data[11], 0);
    data[11] = 14;    
    expect("data", data[11], 14);
    arenaPushFrame(&arena);
    expect("ignroe stack", arena.allocated, 16);
    arenaRelease(&arena);

    PRINT(CC_BLUE, "stackable arena\n");
    arenaInit(&arena, 0, true);
    expect("default size", arena.size, GB(1));
    expect("committed", arena.committed, KB(4));
    expect("allocated", arena.allocated, sizeof(ArenaStackFrame));
    data = arenaAlloc(&arena, 12);
    expect("committed after alloc", arena.committed, KB(4));
    expect("allocated after alloc", arena.allocated, sizeof(ArenaStackFrame) + 16);
    expect("zero data", data[11], 0);
    data[11] = 14;    
    expect("data", data[11], 14);
    arenaReset(&arena);
    expect("committed after reset", arena.committed, KB(4));
    expect("allocated after reset", arena.allocated, sizeof(ArenaStackFrame));
    expect("data after reset", data[11], 14);
    data = arenaAllocRaw(&arena, 12);
    expect("data after raw", data[11], 14);
    arenaReset(&arena);
    data = arenaAlloc(&arena, 12);
    expect("data after reset and alloc", data[11], 0);

    PRINT(CC_BLUE, "stack the arena\n");
    arenaPushFrame(&arena);
    expect("allocated after push frame", arena.allocated, (sizeof(ArenaStackFrame) * 2) + 16);
    arenaAlloc(&arena, 1);
    expect("allocated after alloc", arena.allocated, (sizeof(ArenaStackFrame) * 2) + 24);
    arenaPushFrame(&arena);
    expect("allocated after second push frame", arena.allocated, (sizeof(ArenaStackFrame) * 3) + 24);
    arenaPopFrame(&arena);
    expect("allocated after pop", arena.allocated, (sizeof(ArenaStackFrame) * 2) + 24);
    arenaPopFrame(&arena);
    expect("allocated after second pop", arena.allocated, (sizeof(ArenaStackFrame) * 1) + 16);
    arenaPopFrame(&arena);
    expect("allocated after third pop", arena.allocated, (sizeof(ArenaStackFrame) * 1));
    arenaPopFrame(&arena);
    expect("allocated after fourth pop", arena.allocated, (sizeof(ArenaStackFrame) * 1));
    arenaPushFrame(&arena);
    expect("allocated after push", arena.allocated, (sizeof(ArenaStackFrame) * 2));
    arenaPopFrame(&arena);
    expect("allocated after pop", arena.allocated, (sizeof(ArenaStackFrame) * 1));
    arenaRelease(&arena);

    PRINT(CC_BLUE, "arena inside arena\n");
    Arena* arenaPtr = arenaNew(1, true);
    expect("arenaptr", arenaPtr, arenaPtr->data);
    expect("size", arenaPtr->size, KB(4));
    expect("committed", arenaPtr->committed, KB(4));
    expect("allocated", arenaPtr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));
    data = arenaAlloc(arenaPtr, 1);
    expect("data ptr", data, arenaPtr->data + sizeof(Arena) + sizeof(ArenaStackFrame));
    expect("committed after alloc", arenaPtr->committed, KB(4));
    expect("allocated after alloc", arenaPtr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame) + 8);
    expect("zero data", *data, 0);
    *data = 14;    
    expect("data", *data, 14);
    arenaReset(arenaPtr);
    expect("allocated after reset", arenaPtr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));
    arenaPushFrame(arenaPtr);
    expect("allocated after push", arenaPtr->allocated, sizeof(Arena) + (sizeof(ArenaStackFrame) * 2));
    arenaPopFrame(arenaPtr);
    expect("allocated after pop", arenaPtr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));
    arenaPopFrame(arenaPtr);
    expect("allocated after second pop", arenaPtr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));

    PRINT(CC_BLUE, "arena children\n");
    Arena* child = arenaAddChild(arenaPtr, 100, false);
    PRINT(CC_YELLOW, "Child 1: %lu\n", child);
    expect("size", child->size, KB(4));
    expect("committed", child->committed, 0);
    expect("allocated", child->allocated, 0);
    child = arenaAddChild(arenaPtr, 100, true);
    PRINT(CC_YELLOW, "Child 2: %lu\n", child);
    expect("size", child->size, KB(4));
    expect("committed", child->committed, KB(4));
    expect("allocated", child->allocated, sizeof(ArenaStackFrame));
    arenaRelease(arenaPtr);


    PRINT(CC_BLUE, "================\n Report\n================\n");
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}