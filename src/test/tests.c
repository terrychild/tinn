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
    Arena* arena_ptr = arenaNew(1, true);
    expect("arenaptr", arena_ptr, arena_ptr->data);
    expect("size", arena_ptr->size, KB(4));
    expect("committed", arena_ptr->committed, KB(4));
    expect("allocated", arena_ptr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));
    data = arenaAlloc(arena_ptr, 1);
    expect("data ptr", data, arena_ptr->data + sizeof(Arena) + sizeof(ArenaStackFrame));
    expect("committed after alloc", arena_ptr->committed, KB(4));
    expect("allocated after alloc", arena_ptr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame) + 8);
    expect("zero data", *data, 0);
    *data = 14;    
    expect("data", *data, 14);
    arenaReset(arena_ptr);
    expect("allocated after reset", arena_ptr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));
    arenaPushFrame(arena_ptr);
    expect("allocated after push", arena_ptr->allocated, sizeof(Arena) + (sizeof(ArenaStackFrame) * 2));
    arenaPopFrame(arena_ptr);
    expect("allocated after pop", arena_ptr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));
    arenaPopFrame(arena_ptr);
    expect("allocated after second pop", arena_ptr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));

    PRINT(CC_BLUE, "arena children\n");
    Arena* child = arenaAddChild(arena_ptr, 100, false);
    PRINT(CC_YELLOW, "Child 1: %lu\n", child);
    expect("size", child->size, KB(4));
    expect("committed", child->committed, 0);
    expect("allocated", child->allocated, 0);
    child = arenaAddChild(arena_ptr, 100, true);
    PRINT(CC_YELLOW, "Child 2: %lu\n", child);
    expect("size", child->size, KB(4));
    expect("committed", child->committed, KB(4));
    expect("allocated", child->allocated, sizeof(ArenaStackFrame));
    arenaReset(arena_ptr);


    PRINT(CC_BLUE, "================\n Array tests\n================\n");
    arenaPushFrame(arena_ptr);

    Array* array = arrayNew(arena_ptr, 1, 10, 0);
    expect("allocated capacity (10 * U8)", array->capacity, 10);
    
    array = arrayNew(arena_ptr, sizeof(U64), 4, 0);
    expect("allocated capacity (4 * U64)", array->capacity, 4);
    expect("empty", array->count, 0);

    U64 nums[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    expect("added one", arrayPush(array, &nums[0]), 1);
    expect("get one", *((U64*)arrayGet(array, 0)), nums[0]);

    arrayPush(array, &nums[1]);
    arrayPush(array, &nums[2]);
    arrayPush(array, &nums[3]);
    expect("added four", array->count, 4);
    expect("capacity after four", array->capacity, 4);

    arrayPush(array, &nums[4]);
    expect("added five", array->count, 5);
    expect("capacity after five", array->capacity, 8);

    arrayPush(array, &nums[5]);
    arrayPush(array, &nums[6]);
    arrayPush(array, &nums[7]);
    expect("added eitgh", array->count, 8);
    expect("capacity after eight", array->capacity, 8);

    arrayPush(array, &nums[8]);
    expect("added nine", array->count, 9);
    expect("capacity after nine", array->capacity, 16);

    expect("get 4", *((U64*)arrayGet(array, 4)), nums[4]);
    expect("get 8", *((U64*)arrayGet(array, 8)), nums[8]);

    U64* arrayPtr = (U64*)array->start;
    expect("pointer syntax", *arrayPtr, nums[0]);
    expect("pointer arithmetic", *(arrayPtr+4), nums[4]);
    expect("array syntax", arrayPtr[8], nums[8]);

    arrayRemove(array, 8);
    expectNull("remove 8", arrayGet(array, 8));
    expect("get 7", *((U64*)arrayGet(array, 7)), nums[7]);

    expect("get 4", *((U64*)arrayGet(array, 4)), nums[4]);
    arrayRemove(array, 4);
    expect("remove 4", *((U64*)arrayGet(array, 4)), nums[5]);
    expectNull("remove 7", arrayGet(array, 7));
    expect("get 6", *((U64*)arrayGet(array, 6)), nums[7]);

    arrayRemove(array, 0);
    expect("remove 0", *((U64*)arrayGet(array, 0)), nums[1]);

    expect("pop", *((U64*)arrayPop(array)), nums[7]);
    expect("after pop", array->count, 5);

    arraySet(array, 0, &nums[0]);
    expect("set 0", *((U64*)arrayGet(array, 0)), nums[0]);
    arraySet(array, 1, &nums[1]);
    expect("set 1", *((U64*)arrayGet(array, 1)), nums[1]);

    arenaPopFrame(arena_ptr);


    PRINT(CC_BLUE, "================\n Pool tests\n================\n");
    arenaPushFrame(arena_ptr);

    Pool* pool = poolNew(arena_ptr, sizeof(U64), 4, 0);
    expect("capacity", pool->capacity, 4);
    poolDebug(pool);
    U64* p0 = poolPush(pool, &nums[0]);
    expect("add 1", pool->count, 1);
    poolDebug(pool);
    U64* p1 = poolPush(pool, &nums[1]);
    expect("add 2", pool->count, 2);
    poolDebug(pool);
    U64* p2 = poolPush(pool, &nums[2]);
    expect("add 3", pool->count, 3);
    poolDebug(pool);
    U64* p3 = poolPush(pool, &nums[3]);
    expect("add 4", pool->count, 4);
    poolDebug(pool);
    U64* p4 = poolPush(pool, &nums[4]);
    expect("capacity after one more", pool->capacity, 8);
    poolDebug(pool);

    U64* p14 = poolAdd(pool);
    expect("add", pool->count, 6);
    expect("blank", *p14, 0);
    *p14 = 14;
    expect("after set", *p14, 14);
    poolDebug(pool);

    poolRemove(pool, p1);
    expect("count after remove 1", pool->count, 5);
    poolDebug(pool);

    poolRemove(pool, p3);
    expect("count after remove 3", pool->count, 4);
    poolDebug(pool);

    poolRemove(pool, p14);
    expect("count after remove 14", pool->count, 3);
    poolDebug(pool);

    poolRemove(pool, p0);
    expect("count after remove 0", pool->count, 2);
    poolDebug(pool);

    poolRemove(pool, p2);
    expect("count after remove 2", pool->count, 1);
    poolDebug(pool);

    poolRemove(pool, p2);
    expect("count after double remove 2", pool->count, 1);
    poolDebug(pool);

    poolRemove(pool, &nums[1]);
    expect("count after remove of invalid", pool->count, 1);
    poolDebug(pool);

    poolRemove(pool, p4);
    expect("count after remove 4", pool->count, 0);
    poolDebug(pool);

    p1 = poolPush(pool, &nums[1]);
    expect("count after add 1", pool->count, 1);
    poolDebug(pool);

    arenaPopFrame(arena_ptr);


    PRINT(CC_BLUE, "================\n Buffer tests\n================\n");
    arenaPushFrame(arena_ptr);

    Buffer* buf = bufNew(arena_ptr, 8, 0);
    expect("length", buf->length, 0);
    expect("size", buf->size, 8);

    bufAppend(buf, (U8*)"hello world", 5);
    expect("Appeding data", bufAsStr(buf), "hello");
    expect("length", buf->length, 5);
    expect("size", buf->size, 8);

    bufAppendStr(buf, " world!");
    expect("Appeding a string", bufAsStr(buf), "hello world!");
    expect("length", buf->length, 12);
    expect("size", buf->size, 16);

    bufReset(buf);
    expect("reseting", bufAsStr(buf), "");
    expect("length", buf->length, 0);
    expect("size", buf->size, 16);

    BufferSpace space = bufReadyWrite(buf, 12);
    expect("space length", space.length, 16);
    expect("space start", space.start, buf->start);
    expect("length", buf->length, 0);
    expect("size", buf->size, 16);

    for (int i=0; i<12; i++) {
        space.start[i] = 65+i;
    }
    bufConfirmWrite(buf, 12);
    expect("direct write", bufAsStr(buf), "ABCDEFGHIJKL");
    expect("length", buf->length, 12);
    expect("size", buf->size, 16);

    space = bufReadyWrite(buf, 14);
    expect("space length", space.length, 20);
    expect("length", buf->length, 12);
    expect("size", buf->size, 32);
    for (int i=0; i<14; i++) {
        space.start[i] = 65+12+i;
    }
    bufConfirmWrite(buf, 14);
    expect("direct write 2", bufAsStr(buf), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    expect("length", buf->length, 26);
    expect("size", buf->size, 32);

    bufHexDump(buf);

    arenaPopFrame(arena_ptr);


    PRINT(CC_BLUE, "================\n Report\n================\n");
    expect("final arena check", arena_ptr->allocated, sizeof(Arena) + sizeof(ArenaStackFrame));
    arenaRelease(arena_ptr);
    
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}