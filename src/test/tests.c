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

    // arena tests
    PRINT(CC_BLUE, "Arena Allocator tests\n");

    ArenaAllocator arena;
    arenaInit(&arena, 1);
    expect("page size", arena.size, KB(4));
    expect("committed", arena.committed, 0);
    expect("allocated", arena.allocated, 0);
    arenaRelease(&arena);

    arenaInit(&arena, 0);
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
    PRINT(CC_BLUE, "Dynamic Array tests\n");

    DynamicArray da;
    daInit(&da, 1, 10, 0);
    expect("allocated capacity (10 * U8)", da.capacity, 16);
    daRelease(&da);

    daInit(&da, sizeof(U64), 4, 0);
    expect("allocated capacity (4 * U64)", da.capacity, 4);
    expect("empty", da.count, 0);

    U64 one = 1;
    daPush(&da, &one);
    expect("added one", da.count, 1);
    expect("get one", *((U64*)daGet(&da, 0)), one);

    daPush(&da, &one);
    daPush(&da, &one);
    daPush(&da, &one);
    expect("added four", da.count, 4);
    expect("capacity after four", da.capacity, 4);

    U64 five = 5;
    daPush(&da, &five);
    expect("added five", da.count, 5);
    expect("capacity after five", da.capacity, 8);

    daPush(&da, &one);
    daPush(&da, &one);
    daPush(&da, &one);
    expect("added eitgh", da.count, 8);
    expect("capacity after eight", da.capacity, 8);

    U64 nine = 9;
    daPush(&da, &nine);
    expect("added nine", da.count, 9);
    expect("capacity after nine", da.capacity, 16);

    expect("get five", *((U64*)daGet(&da, 4)), five);
    expect("get nine", *((U64*)daGet(&da, 8)), nine);

    U64* daPtr = (U64*)da.data;
    expect("pointer syntax", *daPtr, one);
    expect("pointer arithmetic", *(daPtr+4), five);
    expect("array syntax", daPtr[8], nine);


    daRelease(&da);

    // report
    if (expect_failed) {
        PRINT(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINT(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}