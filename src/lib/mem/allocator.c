#include <stdlib.h>
#include <string.h>

#include "lib/mem/allocator.h"
#include "lib/mem/align.h"
#include "lib/mem/arena.h"
#include "lib/mem/pool.h"

#include "lib/cli.h"

// children
Allocator* allocateChild(Allocator* allocator) {
    if (!allocator->children) {
        allocator->children = poolNew(allocator, sizeof(Allocator), 8);
    }
    Arena* arena = arenaNew(0);
    Allocator* child = poolAdd(allocator->children);
    allocatorInit(child, arena);
    return child;
}
void deallocateChild(Allocator* allocator, Allocator* child) {
    if (allocator->children) {
        if (poolRemove(allocator->children, child)) {
            allocatorRelease(child);
        }
    }
}

// normal allocation
void* allocate(Allocator* allocator, U64 size) {
    return arenaAlloc(allocator->arena, size);
}

// Allocator
Allocator* allocatorNew() {
    Arena* arena = arenaNew(0);
    Allocator* allocator = arenaAlloc(arena, sizeof(Allocator));
    allocatorInit(allocator, arena);
    return allocator;
}
void allocatorInit(Allocator* allocator, Arena* arena) {
    allocator->arena = arena;
    allocator->children = NULL;
}
void allocatorReset(Allocator* allocator) {
    // release children
    if (allocator->children) {
        PoolNode* node = allocator->children->first;
        while (node != NULL) {
            allocatorRelease((Allocator*)poolData(node));
            node = node->next;
        }
    }

    // reset arena
    const U64 AREAN_SIZE = alignToWord(sizeof(Arena));
    const U64 ALLOCATOR_SIZE = alignToWord(sizeof(Allocator));

    if ((U8*)allocator->arena == allocator->arena->start) {
        if ((U8*)allocator == allocator->arena->start + AREAN_SIZE) {
            allocator->arena->allocated = AREAN_SIZE + ALLOCATOR_SIZE;
        } else {
            allocator->arena->allocated = AREAN_SIZE;
        }
    } else {
        allocator->arena->allocated = 0;
    }

    allocator->children = NULL;
}
void allocatorRelease(Allocator* allocator) {
    arenaRelease(allocator->arena);
}

// debug
static void debug(Allocator* allocator, U8 level) {
    char indent[256];
    for (U8 i=0; i<level; i++) {
        indent[i] = ' ';
    }
    indent[level] = '\0';

    PRINT(CC_MAGENTA, "%sAllocated: %lu, children: %lu\n", indent, allocator->arena->allocated, allocator->children ? allocator->children->count : 0);
    if (allocator->children) {
        PoolNode* node = allocator->children->first;
        while (node != NULL) {
            debug((Allocator*)poolData(node), level+2);
            node = node->next;
        }
    }
}
void allocatorDebug(Allocator* allocator) {
    debug(allocator, 0);
}