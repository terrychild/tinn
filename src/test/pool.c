#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"
#include "lib/mem/pool.h"

void testPool(Allocator* allocator) {
    PRINT(CC_BLUE, "================\n Pool tests\n================\n");

    U64 nums[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    Pool* pool = poolNew(allocator, sizeof(U64), 4);
    expect("capacity", pool->capacity, 4);
    expect("total allocated", pool->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame) + sizeof(Pool) + (4 * (sizeof(PoolNode) + sizeof(U64))));
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

    U64* p6 = poolPush(pool, &nums[6]);
    U64* p7 = poolPush(pool, &nums[7]);
    expect("add two more", pool->count, 8);
    expect("after two more", pool->capacity, 8);
    expect("total allocated", pool->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame) + sizeof(Pool) + (8 * (sizeof(PoolNode) + sizeof(U64))));
    poolDebug(pool);
    allocatorDebug(allocator);

    allocate(allocator, 1);
    expect("allocated something else", pool->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame) + sizeof(Pool) + (8 * (sizeof(PoolNode) + sizeof(U64))) + 8);

    U64* p8 = poolPush(pool, &nums[8]);
    expect("count after extra one", pool->count, 9);
    expect("capacity after extra one", pool->capacity, 16);
    expect("allocated something else", pool->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(AllocatorFrame) + sizeof(Pool) + (16 * (sizeof(PoolNode) + sizeof(U64))) + 8);
    poolDebug(pool);

    poolRemove(pool, p1);
    expect("count after remove 1", pool->count, 8);
    poolDebug(pool);

    poolRemove(pool, p3);
    expect("count after remove 3", pool->count, 7);
    poolDebug(pool);

    poolRemove(pool, p14);
    expect("count after remove 14", pool->count, 6);
    poolDebug(pool);

    poolRemove(pool, p0);
    expect("count after remove 0", pool->count, 5);
    poolDebug(pool);

    poolRemove(pool, p8);
    expect("count after remove 8", pool->count, 4);
    poolDebug(pool);

    poolRemove(pool, p2);
    expect("count after remove 2", pool->count, 3);
    poolDebug(pool);

    poolRemove(pool, p2);
    expect("count after double remove 2", pool->count, 3);
    poolDebug(pool);

    poolRemove(pool, &nums[1]);
    expect("count after remove of invalid", pool->count, 3);
    poolDebug(pool);

    poolRemove(pool, p4);
    poolRemove(pool, p6);
    poolRemove(pool, p7);
    expect("count after remove rest", pool->count, 0);
    poolDebug(pool);

    poolPush(pool, &nums[1]);
    expect("count after add 1", pool->count, 1);
    poolDebug(pool);

    poolPush(pool, &nums[2]);
    poolPush(pool, &nums[3]);
    poolPush(pool, &nums[4]);
    expect("count after add three more", pool->count, 4);
    poolDebug(pool);

    poolReset(pool);
    expect("count after reset", pool->count, 0);
    poolDebug(pool);

    allocatorDebug(allocator);
    allocatorPopFrame(allocator);
}