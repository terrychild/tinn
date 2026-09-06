#include <string.h>
#include <assert.h>

#include "lib/mem/sys.h"
#include "lib/mem/arena.h"
#include "lib/macros.h"
#include "lib/console.h"

#define ARENA_DEFAULT_SIZE GB(1)

ArenaAllocator* arenaNew(U64 arena_size) {
    arena_size = alignToPage(arena_size ? arena_size : ARENA_DEFAULT_SIZE);

    ArenaAllocator* arena;
    U64 struct_size = alignToWord(sizeof(*arena));
    U64 commit_size = alignToPage(struct_size);
    assert(arena_size >= commit_size);

    void* address = sysMemReserve(arena_size);    
    sysMemCommit(address, commit_size);

    arena = (ArenaAllocator*)address;
    arena->data = address;
    arena->size = arena_size;
    arena->committed = commit_size;
    arena->allocated = struct_size;

    return arena;
}
void arenaReset(ArenaAllocator* arena) {
    arena->allocated = alignToWord(sizeof(*arena));
}
void arenaRelease(ArenaAllocator* arena) {
    sysMemRelease(arena->data, arena->size);
}

static void* arenaAllocate(ArenaAllocator* arena, U64 size, bool zero) {
    size = alignToWord(size);

    if (arena->allocated + size > arena->committed) {
        U64 commit_size = alignToPage(size);
        if (arena->committed + commit_size > arena->size) {
            PANIC("Arena is out of memory");
        } else {
            sysMemCommit(arena->data + arena->committed, commit_size);
            arena->committed += commit_size;
        }        
    }

    void* new_data = arena->data + arena->allocated;
    arena->allocated += size;

    if (zero) {
        memset(new_data, 0, size);
    }

    return new_data;
}

void* arenaAlloc(ArenaAllocator* arena, U64 size) {
    return arenaAllocate(arena, size, true);
}

void* arenaAllocRaw(ArenaAllocator* arena, U64 size) {
    return arenaAllocate(arena, size, false);
}