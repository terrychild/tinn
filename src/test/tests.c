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

    Arena arena;
    arenaInit(&arena, 1, &arena_pool);
    expect("page size", arena.size, KB(4));
    expect("committed", arena.committed, 0);
    expect("allocated", arena.allocated, 0);
    expect("arena pool count", arena_pool.pool.count, 1);
    arenaRelease(&arena);
    expect("arena pool count", arena_pool.pool.count, 0);

    arenaInit(&arena, 0, &arena_pool);
    expect("default size", arena.size, GB(1));
    U8* data = arenaAlloc(&arena, 12);
    expect("commit size", arena.committed, KB(4));
    expect("allocate size", arena.allocated, 16);
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
    arenaRelease(&arena);

    // dynamic array tests
    PRINT(CC_BLUE, "Array tests\n");

    Array array;
    arrayInit(&array, 1, 10, 0, &arena_pool);
    expect("allocated capacity (10 * U8)", array.capacity, 16);
    arrayRelease(&array);

    arrayInit(&array, sizeof(U64), 4, 0, &arena_pool);
    expect("allocated capacity (4 * U64)", array.capacity, 4);
    expect("empty", array.count, 0);

    U64 one = 1;
    arrayPush(&array, &one);
    expect("added one", array.count, 1);
    expect("get one", *((U64*)arrayGet(&array, 0)), one);

    arrayPush(&array, &one);
    arrayPush(&array, &one);
    arrayPush(&array, &one);
    expect("added four", array.count, 4);
    expect("capacity after four", array.capacity, 4);

    U64 five = 5;
    arrayPush(&array, &five);
    expect("added five", array.count, 5);
    expect("capacity after five", array.capacity, 8);

    arrayPush(&array, &one);
    arrayPush(&array, &one);
    arrayPush(&array, &one);
    expect("added eitgh", array.count, 8);
    expect("capacity after eight", array.capacity, 8);

    U64 nine = 9;
    arrayPush(&array, &nine);
    expect("added nine", array.count, 9);
    expect("capacity after nine", array.capacity, 16);

    expect("get five", *((U64*)arrayGet(&array, 4)), five);
    expect("get nine", *((U64*)arrayGet(&array, 8)), nine);

    U64* arrayPtr = (U64*)array.data;
    expect("pointer syntax", *arrayPtr, one);
    expect("pointer arithmetic", *(arrayPtr+4), five);
    expect("array syntax", arrayPtr[8], nine);

    arrayRelease(&array);

    // pool tests
    PRINT(CC_BLUE, "Pool tests\n");

    Pool pool;
    poolInit(&pool, 1, 12, 0, &arena_pool);
    expect("capacity (page size)", pool.capacity, 16);
    poolRelease(&pool);

    char a = 'a';
    char b = 'b';
    char c = 'c';
    char d = 'd';

    poolInit(&pool, 1, 12, 12, &arena_pool);
    expect("capacity (specific)", pool.capacity, 12);
    poolAdd(&pool, &a);
    poolAdd(&pool, &b);
    poolAdd(&pool, &c);
    poolAdd(&pool, &d);
    expect("count after four", pool.count, 4);
    expect("get b", *((char*)poolGet(&pool, 1)), b);
    poolRemove(&pool, 1);
    expect("count after remove", pool.count, 3);
    expect("get d", *((char*)poolGet(&pool, 1)), d);

    poolAdd(&pool, &a);
    poolAdd(&pool, &a);
    poolAdd(&pool, &a);
    poolAdd(&pool, &a);
    poolAdd(&pool, &a);
    poolAdd(&pool, &a);
    poolAdd(&pool, &a);
    poolAdd(&pool, &a);
    poolAdd(&pool, &a);
    expect("count after twelve", pool.count, 12);
    poolRemove(&pool, 11);
    expect("count after remove", pool.count, 11);
    expectNull("get NULL", poolGet(&pool, 11));
    //poolAdd(&pool, &d);

    //poolRelease(&pool);

    // free arena pool
    PRINT(CC_BLUE, "Arena Pool tests\n");
    expect("arena pool count", arena_pool.pool.count, 1);
    arenaPoolReset(&arena_pool);
    expect("arena pool count", arena_pool.pool.count, 0);

    Array arr[5];
    arrayInit(&arr[0], 1, 1, 1, &arena_pool);
    arrayInit(&arr[1], 1, 1, 1, &arena_pool);
    arrayInit(&arr[2], 1, 1, 1, &arena_pool);
    arrayInit(&arr[3], 1, 1, 1, &arena_pool);
    expect("arena pool count", arena_pool.pool.count, 4);
    arrayInit(&arr[4], 1, 1, 1, &arena_pool);

    // report
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}