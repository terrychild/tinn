#include <stdlib.h>

#include "test/expect.h"
#include "lib.h"

int runTests() {
    expect_failed = false;

    // type tests
    PRINT(CC_BLUE, "Type tests\n");

    expect("U8 size", sizeof(U8), 1);
    expect("U64 size", sizeof(U64), 8);
    expect("U8 range", (U8)(-1), 255);
    expect("size_t is 64 bit", sizeof(size_t), sizeof(U64));

    // create an arena pool
    ArenaPool arena_pool;
    arenaPoolInit(&arena_pool, 4, 4);

    // arena tests
    PRINT(CC_BLUE, "Arena Allocator tests\n");

    Arena* arena = arenaPoolAdd(&arena_pool, 1);
    expect("page size", arena->size, KB(4));
    expect("committed", arena->committed, 0);
    expect("allocated", arena->allocated, 0);
    expect("arena pool count", arena_pool.count, 1);
    arenaPoolRemove(&arena_pool, arena);
    expect("arena pool count", arena_pool.count, 0);

    arena = arenaPoolAdd(&arena_pool, 0);
    expect("default size", arena->size, GB(1));
    U8* data = arenaAlloc(arena, 12);
    expect("commit size", arena->committed, KB(4));
    expect("allocate size", arena->allocated, 16);
    expect("zero data", data[11], 0);
    data[11] = 14;    
    expect("data", data[11], 14);
    arenaReset(arena);
    expect("data after reset", data[11], 14);
    data = arenaAllocRaw(arena, 12);
    expect("data after raw", data[11], 14);
    arenaReset(arena);
    data = arenaAlloc(arena, 12);
    expect("data after reset and alloc", data[11], 0);

    // dynamic array tests
    PRINT(CC_BLUE, "Array tests\n");

    Array array;
    arrayInit(&array, 1, 10, 0, &arena_pool);
    expect("allocated capacity (10 * U8)", array.capacity, 16);
    arrayRelease(&array);

    arrayInit(&array, sizeof(U64), 4, 0, &arena_pool);
    expect("allocated capacity (4 * U64)", array.capacity, 4);
    expect("empty", array.count, 0);

    U64 nums[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    expect("added one", arrayPush(&array, &nums[0]), 1);
    expect("get one", *((U64*)arrayGet(&array, 0)), nums[0]);

    arrayPush(&array, &nums[1]);
    arrayPush(&array, &nums[2]);
    arrayPush(&array, &nums[3]);
    expect("added four", array.count, 4);
    expect("capacity after four", array.capacity, 4);

    arrayPush(&array, &nums[4]);
    expect("added five", array.count, 5);
    expect("capacity after five", array.capacity, 8);

    arrayPush(&array, &nums[5]);
    arrayPush(&array, &nums[6]);
    arrayPush(&array, &nums[7]);
    expect("added eitgh", array.count, 8);
    expect("capacity after eight", array.capacity, 8);

    arrayPush(&array, &nums[8]);
    expect("added nine", array.count, 9);
    expect("capacity after nine", array.capacity, 16);

    expect("get 4", *((U64*)arrayGet(&array, 4)), nums[4]);
    expect("get 8", *((U64*)arrayGet(&array, 8)), nums[8]);

    U64* arrayPtr = (U64*)array.data;
    expect("pointer syntax", *arrayPtr, nums[0]);
    expect("pointer arithmetic", *(arrayPtr+4), nums[4]);
    expect("array syntax", arrayPtr[8], nums[8]);

    arrayRemove(&array, 8);
    expectNull("remove 8", arrayGet(&array, 8));
    expect("get 7", *((U64*)arrayGet(&array, 7)), nums[7]);

    expect("get 4", *((U64*)arrayGet(&array, 4)), nums[4]);
    arrayRemove(&array, 4);
    expect("remove 4", *((U64*)arrayGet(&array, 4)), nums[5]);
    expectNull("remove 7", arrayGet(&array, 7));
    expect("get 6", *((U64*)arrayGet(&array, 6)), nums[7]);

    arrayRemove(&array, 0);
    expect("remove 0", *((U64*)arrayGet(&array, 0)), nums[1]);

    expect("pop", *((U64*)arrayPop(&array)), nums[7]);
    expect("after pop", array.count, 5);

    arraySet(&array, 0, &nums[0]);
    expect("set 0", *((U64*)arrayGet(&array, 0)), nums[0]);
    arraySet(&array, 1, &nums[1]);
    expect("set 1", *((U64*)arrayGet(&array, 1)), nums[1]);

    // pool tests
    PRINT(CC_BLUE, "Pool tests\n");

    Pool pool;
    poolInit(&pool, sizeof(U64), 4, 0, &arena_pool);
    expect("capacity", pool.capacity, 4);
    poolDebug(&pool);
    U64* p0 = poolPush(&pool, &nums[0]);
    expect("add 1", pool.count, 1);
    poolDebug(&pool);
    U64* p1 = poolPush(&pool, &nums[1]);
    expect("add 2", pool.count, 2);
    poolDebug(&pool);
    U64* p2 = poolPush(&pool, &nums[2]);
    expect("add 3", pool.count, 3);
    poolDebug(&pool);
    U64* p3 = poolPush(&pool, &nums[3]);
    expect("add 4", pool.count, 4);
    poolDebug(&pool);
    U64* p4 = poolPush(&pool, &nums[4]);
    expect("capacity after one more", pool.capacity, 8);
    poolDebug(&pool);

    U64* p14 = poolAdd(&pool);
    expect("add", pool.count, 6);
    expect("blank", *p14, 0);
    *p14 = 14;
    expect("after set", *p14, 14);
    poolDebug(&pool);

    poolRemove(&pool, p1);
    expect("count after remove 1", pool.count, 5);
    poolDebug(&pool);

    poolRemove(&pool, p3);
    expect("count after remove 3", pool.count, 4);
    poolDebug(&pool);

    poolRemove(&pool, p14);
    expect("count after remove 14", pool.count, 3);
    poolDebug(&pool);

    poolRemove(&pool, p0);
    expect("count after remove 0", pool.count, 2);
    poolDebug(&pool);

    poolRemove(&pool, p2);
    expect("count after remove 2", pool.count, 1);
    poolDebug(&pool);

    poolRemove(&pool, p2);
    expect("count after double remove 2", pool.count, 1);
    poolDebug(&pool);

    poolRemove(&pool, &nums[1]);
    expect("count after remove of invalid", pool.count, 1);
    poolDebug(&pool);

    poolRemove(&pool, p4);
    expect("count after remove 4", pool.count, 0);
    poolDebug(&pool);

    p1 = poolPush(&pool, &nums[1]);
    expect("count after add 1", pool.count, 1);
    poolDebug(&pool);

    //poolRelease(&pool);

    // free arena pool
    PRINT(CC_BLUE, "Arena Pool tests\n");
    expect("arena pool count", arena_pool.count, 3);
    arenaPoolReset(&arena_pool);
    expect("arena pool count", arena_pool.count, 0);

    /*Array arr[5];
    arrayInit(&arr[0], 1, 1, 1, &arena_pool);
    arrayInit(&arr[1], 1, 1, 1, &arena_pool);
    arrayInit(&arr[2], 1, 1, 1, &arena_pool);
    arrayInit(&arr[3], 1, 1, 1, &arena_pool);
    expect("arena pool count", arena_pool.pool.count, 4);
    arrayInit(&arr[4], 1, 1, 1, &arena_pool);*/

    // report
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}