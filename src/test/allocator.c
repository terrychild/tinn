#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"
#include "lib/mem/pool.h"

void testAllocator(Allocator* allocator) {
    PRINT(CC_BLUE, "================\n Allocator tests\n================\n");

    expect("size", allocator->arena->size, GB(1));
    expect("committed", allocator->arena->committed, KB(4));
    expect("allocated", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator));
    allocatorDebug(allocator);

    U64* some_data = allocate(allocator, 8);
    *some_data = 14;
    expect("allocate", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Allocation) + 8);
    expect("allocate (data)", *some_data, 14);
    U64* updated_data = reallocate(allocator, some_data, 16);
    expect("reallocate (size)", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Allocation) + 16);
    expect("reallocate (pointers)", updated_data, some_data);
    expect("reallocate (data)", *updated_data, 14);
    U64* other_data = allocate(allocator, 8);
    expect("allocate other thing", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Allocation) + 16 + sizeof(Allocation) + 8);
    updated_data = reallocate(allocator, some_data, 24);
    expect("reallocate again (size)", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Allocation) + 16 + sizeof(Allocation) + 8 + sizeof(Allocation) + 24);
    expectDiff("reallocate again (pointers)", updated_data, some_data);
    expect("reallocate again (data)", *updated_data, 14);

    allocatorReset(allocator);
    expect("clear", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator));
    allocatorDebug(allocator);

    Allocator* child = allocateChild(allocator, 0);
    expect("add child (parent allocated)", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Allocation) + sizeof(Pool) + sizeof(Allocation) + (8 * (sizeof(PoolNode) + sizeof(Allocator))));
    expect("add child (count)", allocator->children->count, 1);
    expect("add child (child allocated)", child->arena->allocated, sizeof(Arena));
    allocatorDebug(allocator);

    allocate(child, 96);
    expect("add data", child->arena->allocated, sizeof(Arena) + sizeof(Allocation) + 96);
    allocatorDebug(allocator);

    deallocateChild(allocator, child);
    expect("remove child (parent allocated)", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Allocation) + sizeof(Pool) + sizeof(Allocation) + (8 * (sizeof(PoolNode) + sizeof(Allocator))));
    expect("remove child (count)", allocator->children->count, 0);
    allocatorDebug(allocator);

    deallocateChild(allocator, child);
    expect("double remove child (count)", allocator->children->count, 0);
    allocatorDebug(allocator);

    allocatorReset(allocator);
    expect("reset", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator));
    allocatorDebug(allocator);
}