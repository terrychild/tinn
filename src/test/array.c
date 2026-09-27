#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/mem/arena.h"

void testArray(Allocator* allocator) {
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
}