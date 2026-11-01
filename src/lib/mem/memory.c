#include <stdlib.h>
#include <string.h>

#include "lib/mem/memory.h"
#include "lib/mem/align.h"
#include "lib/mem/arena.h"
#include "lib/mem/mfile.h"
#include "lib/mem/pool.h"

#include "lib/macros.h"
#include "lib/log.h"
#include "lib/cli.h"

// children
Memory* allocateChild(Memory* mem, U64 size) {
    if (!mem->children) {
        mem->children = poolNew(mem, sizeof(Memory), 8);
    }
    Arena* arena = arenaNew(size);
    Memory* child = poolAdd(mem->children);
    memoryInit(child, arena);
    return child;
}
void deallocateChild(Memory* mem, Memory* child) {
    if (mem->children) {
        if (poolRemove(mem->children, child)) {
            memoryRelease(child);
        }
    }
}

// files
MappedFile* allocateFile(Memory* mem, Polling* polling, const char* path) {
    if (!mem->files) {
        mem->files = poolNew(mem, sizeof(MappedFile), 8);
    }
    MappedFile* file = poolAdd(mem->files);
    if (!mfileOpen(file, polling, path)) {
        poolRemove(mem->files, file);
        return NULL;
    }
    return file;
}
void deallocateFile(Memory* mem, MappedFile* file) {
    if (mem->files) {
        if (poolRemove(mem->files, file)) {
            mfileClose(file);
        }
    }
}

// normal allocation
void* allocate(Memory* mem, U64 size) {
    ArenaAllocation arena_allocation = arenaAllocate(mem->arena, sizeof(Allocation) + size);
    mem->last_allocation = (Allocation*)arena_allocation.start;
    mem->last_allocation->size = arena_allocation.size - sizeof(Allocation);
    return mem->last_allocation->data;
}
void* reallocate(Memory* mem, void* original, U64 size) {
    Allocation* original_allocation = (Allocation*)((U8*)original - sizeof(Allocation));
    if (original_allocation == mem->last_allocation) {
        ArenaAllocation arena_allocation = arenaAllocate(mem->arena, size - original_allocation->size);
        mem->last_allocation->size += arena_allocation.size;
    } else {
        void* new_data = allocate(mem, size);
        memcpy(new_data, original_allocation->data, original_allocation->size);
    }
    return mem->last_allocation->data;
}

// Allocator
Memory* memoryNew(U64 size) {
    Arena* arena = arenaNew(size);
    Memory* mem = arenaAllocate(arena, sizeof(Memory)).start;
    memoryInit(mem, arena);
    return mem;
}
void memoryInit(Memory* mem, Arena* arena) {
    mem->arena = arena;
    mem->children = NULL;
    mem->files = NULL;
}
void memoryReset(Memory* mem) {
    // release children
    if (mem->children) {
        PoolNode* node = mem->children->first;
        while (node != NULL) {
            memoryRelease((Memory*)node->data);
            node = node->next;
        }
    }

    // release files
    if (mem->files) {
        PoolNode* node = mem->files->first;
        while (node != NULL) {
            memoryRelease((Memory*)node->data);
            node = node->next;
        }
    }

    // reset arena
    const U64 AREAN_SIZE = alignToWord(sizeof(Arena));
    const U64 ALLOCATOR_SIZE = alignToWord(sizeof(Memory));

    if ((U8*)mem->arena == mem->arena->start) {
        if ((U8*)mem == mem->arena->start + AREAN_SIZE) {
            mem->arena->allocated = AREAN_SIZE + ALLOCATOR_SIZE;
        } else {
            mem->arena->allocated = AREAN_SIZE;
        }
    } else {
        mem->arena->allocated = 0;
    }

    mem->children = NULL;
    mem->files = NULL;
    mem->last_allocation = NULL;
}
void memoryRelease(Memory* mem) {
    arenaRelease(mem->arena);
}

// Global memory
static Memory* program_memory = NULL;
static Memory* scratch_memory[] = { NULL, NULL };

Memory* programMemory() {
    if (!program_memory) {
        program_memory = memoryNew(GiB(1));
    }
    return program_memory;
}

ScratchMemory scratchMemory(Memory* current) {
    U64 scratch_count = sizeof(scratch_memory)/sizeof(scratch_memory[0]);
    if (!scratch_memory[0]) {
        for (U64 i=0; i<scratch_count; i++) {
            scratch_memory[i] = memoryNew(MiB(512));
        }
    }

    for (U64 i=0; i<scratch_count; i++) {
        if (scratch_memory[i] != current) {
            return (ScratchMemory) {
                .mem = scratch_memory[i],
                .allocated = scratch_memory[i]->arena->allocated
            };
        }
    }

    PANIC("No scratch memory available");
    return (ScratchMemory) { .mem = NULL, .allocated = 0 }; // unreachable
}

void scratchRelease(ScratchMemory scratch) {
    scratch.mem->arena->allocated = scratch.allocated;
}

// debug
static void debug(Memory* mem, U8 level) {
    char indent[256];
    for (U8 i=0; i<level; i++) {
        indent[i] = ' ';
    }
    indent[level] = '\0';

    PRINT(CC_MAGENTA, "%sAllocated: %lu, children: %lu\n", indent, mem->arena->allocated, mem->children ? mem->children->count : 0);
    if (mem->children) {
        PoolNode* node = mem->children->first;
        while (node != NULL) {
            debug((Memory*)node->data, level+2);
            node = node->next;
        }
    }
}
void memoryDebug(Memory* mem) {
    debug(mem, 0);
}