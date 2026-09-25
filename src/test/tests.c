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
    arenaInit(&arena, 1);
    expect("page size", arena.size, KB(4));
    expect("committed", arena.committed, 0);
    expect("allocated", arena.allocated, 0);
    U8* data = arenaAlloc(&arena, 12);
    expect("committed after alloc", arena.committed, KB(4));
    expect("allocated after alloc", arena.allocated, 16);
    expect("zero data", data[11], 0);
    data[11] = 14;    
    expect("data", data[11], 14);
    arenaRelease(&arena);

    PRINT(CC_BLUE, "arena inside arena\n");
    Arena* arena_ptr = arenaNew(1);
    expect("arenaptr", arena_ptr, arena_ptr->start);
    expect("size", arena_ptr->size, KB(4));
    expect("committed", arena_ptr->committed, KB(4));
    expect("allocated", arena_ptr->allocated, sizeof(Arena));
    data = arenaAlloc(arena_ptr, 1);
    expect("data ptr", data, arena_ptr->start + sizeof(Arena));
    expect("committed after alloc", arena_ptr->committed, KB(4));
    expect("allocated after alloc", arena_ptr->allocated, sizeof(Arena) + 8);
    expect("zero data", *data, 0);
    *data = 14;    
    expect("data", *data, 14);
    arenaReset(arena_ptr);
    expect("allocated after reset", arena_ptr->allocated, sizeof(Arena));
    arenaRelease(arena_ptr);

    PRINT(CC_BLUE, "================\n Allocator tests\n================\n");

    Allocator* allocator = allocatorNew();
    expect("size", allocator->arena->size, GB(1));
    expect("committed", allocator->arena->committed, KB(4));
    expect("allocated", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame)); 
    allocatorDebug(allocator);

    allocatorPushFrame(allocator);
    expect("add frame", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + (2 * sizeof(AllocatorFrame)) );
    allocatorDebug(allocator);
    allocate(allocator, 96);
    expect("add data", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + (2 * sizeof(AllocatorFrame)) + 96);
    allocatorDebug(allocator);
    allocatorPopFrame(allocator);
    expect("pop frame", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame));
    allocatorDebug(allocator);
    allocatorPopFrame(allocator);
    expect("pop bottom", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame));
    allocatorDebug(allocator);

    allocate(allocator, 96);
    expect("add data", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame) + 96);
    allocatorPopFrame(allocator);
    expect("pop bottom", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame));
    allocate(allocator, 96);
    allocatorPushFrame(allocator);
    allocate(allocator, 48);
    expect("add data, frame, more data", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame) + 96 + sizeof(AllocatorFrame) + 48);
    allocatorDebug(allocator);
    allocatorReset(allocator);    
    expect("reset", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame));
    
    
    PRINT(CC_BLUE, "================\n Array tests\n================\n");
    
    Array* array = arrayNew(allocator, 1, 10, 0);
    expect("allocated capacity (10 * U8)", array->capacity, 10);
    
    array = arrayNew(allocator, sizeof(U64), 4, 0);
    expect("allocated capacity (4 * U64)", array->capacity, 4);
    expect("empty", array->count, 0);

    U64 nums[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    arrayPush(array, &nums[0]);
    expect("added one", array->count, 1);
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
    expect("arena size", array->arena->allocated, 16 * 8);

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

    allocatorDebug(allocator);
    allocatorPopFrame(allocator);


    PRINT(CC_BLUE, "================\n Pool tests\n================\n");
    
    Pool* pool = poolNew(allocator, sizeof(U64), 4, 0);
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

    allocatorDebug(allocator);
    allocatorPopFrame(allocator);


    PRINT(CC_BLUE, "================\n Buffer tests\n================\n");
    
    Buffer* buf = bufNew(allocator, 8, 0);
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

    hexDump(bufAsSlice(buf));

    allocatorDebug(allocator);
    allocatorPopFrame(allocator);

    allocate(allocator, 48);
    buf = bufNew(allocator, 8, 0);
    bufAppendStr(buf, "more testing");
    expect("allocator with data and buf", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame) + 48 + sizeof(Arena) + sizeof(Pool) + sizeof(Buffer));
    allocatorDebug(allocator);
    buf = bufNew(allocator, 8, 0);
    bufAppendStr(buf, "double buffer time!");
    expect("allocator with data and two buffers", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame) + 48 + sizeof(Arena) + sizeof(Pool) + sizeof(Buffer) + sizeof(Buffer));
    allocatorDebug(allocator);
    allocatorPopFrame(allocator);


    PRINT(CC_BLUE, "================\n Report\n================\n");
    expect("Final allocator check", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame)); 
    allocatorDebug(allocator);
    
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}