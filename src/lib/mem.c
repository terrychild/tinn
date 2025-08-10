#include <unistd.h>
#include <sys/mman.h>

#include "lib/mem.h"
#include "lib/console.h"

// align pointers/lengths to page/word boundaries
static bool isPowerOfTwo(u64 a) {
    return (a & (a-1)) == 0;
}
static u64 align(u64 ptr, u64 multiple) {
    if (!isPowerOfTwo(multiple)) {
        PANIC("alignment is not a power of two");
    }

    u64 mod = ptr & (multiple - 1);
    if (mod != 0) {
        ptr += multiple - mod;
    }
    return ptr;
}
static u64 alignPage(u64 ptr) {
    return align(ptr, sysconf(_SC_PAGE_SIZE));
}
static u64 alignWord(u64 ptr) {
    return align(ptr, sizeof(void*));
}

// system calls
// TODO: support more than linux?
static void* sysMemReserve(u64 size) {
    void* mem = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        PANIC("unable to allocate memory");
    }
    return mem;
}

static void sysMemCommit(void* memory, u64 size) {
    if (mprotect(memory, size, PROT_READ | PROT_WRITE)) {
        PANIC("unable to commit memory");
    }
}

static void sysMemUncommit(void* memory, u64 size) {
    if (mprotect(memory, size, PROT_NONE)) {
        PANIC("unable to uncommit memory");
    }
}

static void sysMemRelease(void* memory, u64 size) {
    if (munmap(memory, size)) {
        ERROR("unable to free memory");
    }
}

// Arena Allocator
void arenaInit(ArenaAllocator* arena, u64 size) {
    size = alignPage(size);
    arena->data = sysMemReserve(size);
    arena->size = size;
    arena->committed = 0;
    arena->allocated = 0;
}

void arenaFree(ArenaAllocator* arena) {
    sysMemRelease(arena->data, arena->size);
}

/*void* allocate(void* data, u64 size) {
    void* new_data = realloc(data, size);
    if (new_data == NULL) {
        PANIC("unable to allocate memory");
    }
    return new_data;
}*/