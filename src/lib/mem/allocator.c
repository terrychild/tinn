#include <stdlib.h>
#include <string.h>

#include "lib/mem/allocator.h"
#include <lib/mem/arena.h>
#include <lib/mem/pool.h>

#include <lib/cli.h>

static constexpr U64 CHILDREN_PER_FRAME = 16;
static constexpr U64 ARENAS_PER_FRAME = 16;

// frames
static AllocatorFrame* frameAdd(Allocator* allocator, AllocatorFrame* next) {
    AllocatorFrame* frame = arenaAlloc(allocator->arena, sizeof(AllocatorFrame));
    frame->next = next;
    frame->children = NULL;
    frame->arenas = NULL;
    return frame;
}
static U8* frameRelease(AllocatorFrame* frame) {
    if (frame->children) {
        PoolNode* node = frame->children->first;
        while (node != NULL) {
            allocatorRelease((Allocator*)poolData(node));
            node = node->next;
        }
    }

    if (frame->arenas) {
        PoolNode* node = frame->arenas->first;
        while (node != NULL) {
            arenaRelease((Arena*)poolData(node));
            node = node->next;
        }
    }

    return (U8*)frame;
}
static U8* frameReleaseAll(Allocator* allocator) {
    AllocatorFrame* frame = allocator->top;
    U8* bottom;
    do {
        bottom = frameRelease(frame);
        frame = frame->next;
    } while (frame);
    return bottom;
}

void allocatorPushFrame(Allocator* allocator) {
    AllocatorFrame* frame = frameAdd(allocator, allocator->top);
    allocator->top = frame;
}
void allocatorPopFrame(Allocator* allocator) {
    allocator->arena->allocated = frameRelease(allocator->top) - allocator->arena->start;
    allocator->top = allocator->top->next;
    if (!allocator->top) {
        allocator->top = frameAdd(allocator, NULL);
    }
}

// children
Allocator* allocateChild(Allocator* allocator) {
    if (!allocator->top->children) {
        Arena* children_arena = arenaAlloc(allocator->arena, sizeof(Arena));
        arenaInit(children_arena, 0);
        allocator->top->children = arenaAlloc(allocator->arena, sizeof(Pool));
        poolInit(allocator->top->children, children_arena, sizeof(Allocator), CHILDREN_PER_FRAME);
    }
    Arena* arena = arenaNew(0);
    Allocator* child = poolAdd(allocator->top->children);
    allocatorInit(child, arena);
    return child;
}
void deallocateChild(Allocator* allocator, Allocator* child) {
    if (allocator->top->children) {
        poolRemove(allocator->top->children, child);
    }
}

// sub arenas
Arena* allocateArena(Allocator* allocator, U64 size) {
    if (!allocator->top->arenas) {
        Arena* arenas_arena = arenaAlloc(allocator->arena, sizeof(Arena));
        arenaInit(arenas_arena, 0);
        allocator->top->arenas = arenaAlloc(allocator->arena, sizeof(Pool));
        poolInit(allocator->top->arenas, arenas_arena, sizeof(Arena), ARENAS_PER_FRAME);
    }
    Arena* arena = poolAdd(allocator->top->arenas);
    arenaInit(arena, size);
    return arena;
}
void deallocateArena(Allocator* allocator, Arena* arena) {
    if (allocator->top->arenas) {
        poolRemove(allocator->top->arenas, arena);
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
    allocator->top = frameAdd(allocator, NULL);
}
void allocatorReset(Allocator* allocator) {
    allocator->arena->allocated = frameReleaseAll(allocator) - allocator->arena->start;
    allocator->top = frameAdd(allocator, NULL);
}
void allocatorRelease(Allocator* allocator) {
    frameReleaseAll(allocator);
    arenaRelease(allocator->arena);
}

// debug
static void debug(Allocator* allocator, U8 level) {
    char indent[256];
    for (U8 i=0; i<level; i++) {
        indent[i] = ' ';
    }
    indent[level] = '\0';

    PRINT(CC_MAGENTA, "%sAllocator, allocated: %lu\n", indent, allocator->arena->allocated);
    U64 end = allocator->arena->allocated;
    AllocatorFrame* frame = allocator->top;
    while (frame) {
        U64 start = (U8*)frame - allocator->arena->start;
        U64 size = end - start - sizeof(AllocatorFrame);
        PRINT(CC_MAGENTA, "%s  Frame, allocated: %lu, children: %lu, arenas: %lu\n", 
            indent, 
            size, 
            frame->children ? frame->children->count : 0, 
            frame->arenas ? frame->arenas->count : 0
        );

        if (frame->children) {
            PoolNode* node = frame->children->first;
            while (node != NULL) {
                debug((Allocator*)poolData(node), level+4);
                node = node->next;
            }
        }

        if (frame->arenas) {
            PoolNode* node = frame->arenas->first;
            while (node != NULL) {
                PRINT(CC_MAGENTA, "%s    Arena, allocated: %lu\n", indent, ((Arena*)poolData(node))->allocated);
                node = node->next;
            }
        }

        end = start;
        frame = frame->next;
    }
}
void allocatorDebug(Allocator* allocator) {
    debug(allocator, 0);
}