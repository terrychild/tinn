#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/arena.h"

void testArena() {
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
}