#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "lib/mem/arena.h"
#include "lib/macros.h"
#include "lib/log.h"

static constexpr U64 ARENA_DEFAULT_SIZE = GB(1);

// align pointers/lengths to page/word boundaries
static bool isPowerOfTwo(U64 ptr) {
    return (ptr & (ptr-1)) == 0;
}
static U64 align(U64 ptr, U64 multiple) {
    if (!isPowerOfTwo(multiple)) {
        PANIC("Alignment is not a power of two");
    }

    U64 mod = ptr & (multiple - 1);
    if (mod != 0) {
        ptr += multiple - mod;
    }
    return ptr;
}
static U64 alignToPage(U64 ptr) {
    return align(ptr, sysconf(_SC_PAGE_SIZE));
}
static U64 alignToWord(U64 ptr) {
    return align(ptr, sizeof(void*));
}

// system calls
// TODO: support more than linux?
static void* sysMemReserve(U64 size) {
    void* mem = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        PANIC("Unable to allocate memory");
    }
    return mem;
}

static void sysMemCommit(void* memory, U64 size) {
    if (mprotect(memory, size, PROT_READ | PROT_WRITE)) {
        PANIC("Unable to commit memory");
    }
}

static void sysMemRelease(void* memory, U64 size) {
    if (munmap(memory, size)) {
        ERROR("Unable to free memory");
    }
}

// Arena
Arena* arenaNew(U64 size) {
    Arena temp_arena;
    arenaInit(&temp_arena, size);
    Arena* arena = arenaAlloc(&temp_arena, sizeof(Arena));
    memcpy(arena, &temp_arena, sizeof(Arena));
    return arena;
}
void arenaInit(Arena* arena, U64 size) {
    arena->size = alignToPage(size ? size : ARENA_DEFAULT_SIZE);
    arena->committed = 0;
    arena->allocated = 0;
    arena->start = sysMemReserve(arena->size);
}
void arenaReset(Arena* arena) {
    if ((U8*)arena == arena->start) {
        arena->allocated = alignToWord(sizeof(Arena));
    } else {
        arena->allocated = 0;
    }
}
void arenaRelease(Arena* arena) {
    sysMemRelease(arena->start, arena->size);
}

static void* arenaAllocate(Arena* arena, U64 size, bool zero) {
    size = alignToWord(size);

    if (arena->allocated + size > arena->committed) {
        U64 commit_size = alignToPage(size);
        if (arena->committed + commit_size > arena->size) {
            PANIC("Arena is out of memory");
        } else {
            sysMemCommit(arena->start + arena->committed, commit_size);
            arena->committed += commit_size;
        }        
    }

    void* new_data = arena->start + arena->allocated;
    arena->allocated += size;

    if (zero) {
        memset(new_data, 0, size);
    }

    return new_data;
}
void* arenaAlloc(Arena* arena, U64 size) {
    return arenaAllocate(arena, size, true);
}
void* arenaAllocRaw(Arena* arena, U64 size) {
    return arenaAllocate(arena, size, false);
}