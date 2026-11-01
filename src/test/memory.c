#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/memory.h"
#include "lib/mem/arena.h"
#include "lib/mem/pool.h"

void testMemory(Memory* mem) {
    PRINT(CC_BLUE, "================\n Memory tests\n================\n");

    expect("size", mem->arena->size, GB(1));
    expect("committed", mem->arena->committed, KB(4));
    expect("allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory));
    memoryDebug(mem);

    U64* some_data = allocate(mem, 8);
    *some_data = 14;
    expect("allocate", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + 8);
    expect("allocate (data)", *some_data, 14);
    U64* updated_data = reallocate(mem, some_data, 16);
    expect("reallocate (size)", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + 16);
    expect("reallocate (pointers)", updated_data, some_data);
    expect("reallocate (data)", *updated_data, 14);
    U64* other_data = allocate(mem, 8);
    expect("allocate other thing", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + 16 + sizeof(Allocation) + 8);
    updated_data = reallocate(mem, some_data, 24);
    expect("reallocate again (size)", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + 16 + sizeof(Allocation) + 8 + sizeof(Allocation) + 24);
    expectDiff("reallocate again (pointers)", updated_data, some_data);
    expect("reallocate again (data)", *updated_data, 14);

    memoryReset(mem);
    expect("clear", mem->arena->allocated, sizeof(Arena) + sizeof(Memory));
    memoryDebug(mem);

    Memory* child = allocateChild(mem, 0);
    expect("add child (parent allocated)", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Pool) + sizeof(Allocation) + (8 * (sizeof(PoolNode) + sizeof(Memory))));
    expect("add child (count)", mem->children->count, 1);
    expect("add child (child allocated)", child->arena->allocated, sizeof(Arena));
    memoryDebug(mem);

    allocate(child, 96);
    expect("add data", child->arena->allocated, sizeof(Arena) + sizeof(Allocation) + 96);
    memoryDebug(mem);

    deallocateChild(mem, child);
    expect("remove child (parent allocated)", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Pool) + sizeof(Allocation) + (8 * (sizeof(PoolNode) + sizeof(Memory))));
    expect("remove child (count)", mem->children->count, 0);
    memoryDebug(mem);

    deallocateChild(mem, child);
    expect("double remove child (count)", mem->children->count, 0);
    memoryDebug(mem);

    memoryReset(mem);
    expect("reset", mem->arena->allocated, sizeof(Arena) + sizeof(Memory));
    memoryDebug(mem);
}