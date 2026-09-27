#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"

void testAllocator(Allocator* allocator) {
    PRINT(CC_BLUE, "================\n Allocator tests\n================\n");
    
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
}