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

    Allocator* child = allocateChild(allocator, 0);
    expect("add child (parent allocated)", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Pool) + (8 * (sizeof(PoolNode) + sizeof(Allocator))));
    expect("add child (count)", allocator->children->count, 1);
    expect("add child (child allocated)", child->arena->allocated, sizeof(Arena));
    allocatorDebug(allocator);

    allocate(child, 96);
    expect("add data", child->arena->allocated, sizeof(Arena) + 96);
    allocatorDebug(allocator);

    deallocateChild(allocator, child);
    expect("remove child (parent allocated)", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Pool) + (8 * (sizeof(PoolNode) + sizeof(Allocator))));
    expect("remove child (count)", allocator->children->count, 0);
    allocatorDebug(allocator);

    deallocateChild(allocator, child);
    expect("double remove child (count)", allocator->children->count, 0);
    allocatorDebug(allocator);

    allocatorReset(allocator);
    expect("reset", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator));
    allocatorDebug(allocator);
}