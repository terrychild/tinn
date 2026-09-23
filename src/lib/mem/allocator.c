#include <stdlib.h>
#include <string.h>

#include "lib/mem/allocator.h"
#include <lib/mem/arena.h>
#include <lib/mem/pool.h>

#include <lib/cli.h>

#define CHILDREN_PER_FRAME 16
#define ARENAS_PER_FRAME 16

// frames
static AllocatorFrame* frameAdd(Arena* arena, AllocatorFrame* next) {
    AllocatorFrame* frame = arenaAlloc(arena, sizeof(AllocatorFrame));
    frame->next = next;

    Arena* children_arena = arenaAlloc(arena, sizeof(Arena));
    arenaInit(children_arena, 0);
    frame->children = arenaAlloc(arena, sizeof(Pool));
    poolInit(frame->children, children_arena, sizeof(Allocator), CHILDREN_PER_FRAME);

    Arena* arenas_arena = arenaAlloc(arena, sizeof(Arena));
    arenaInit(arenas_arena, 0);
    frame->arenas = arenaAlloc(arena, sizeof(Pool));
    poolInit(frame->arenas, arenas_arena, sizeof(Arena), ARENAS_PER_FRAME);

    return frame;
}
static U8* frameRelease(AllocatorFrame* frame) {
    PoolNode* node = frame->children->first;
    while (node != NULL) {
        allocatorRelease((Allocator*)poolData(node));
        node = node->next;
    }

    node = frame->arenas->first;
    while (node != NULL) {
        arenaRelease((Arena*)poolData(node));
        node = node->next;
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

// Allocator
Allocator* allocatorNew() {
    Arena* arena = arenaNew(0);
    Allocator* allocator = arenaAlloc(arena, sizeof(Allocator));
    allocatorInit(allocator, arena);
    return allocator;
}
void allocatorInit(Allocator* allocator, Arena* arena) {
    allocator->arena = arena;
    allocator->top = frameAdd(allocator->arena, NULL);
}
void allocatorReset(Allocator* allocator) {
    allocator->arena->allocated = frameReleaseAll(allocator) - allocator->arena->start;
    allocator->top = frameAdd(allocator->arena, NULL);
}
void allocatorRelease(Allocator* allocator) {
    frameReleaseAll(allocator);
    arenaRelease(allocator->arena);
}

void allocatorPushFrame(Allocator* allocator) {
    AllocatorFrame* frame = frameAdd(allocator->arena, allocator->top);
    allocator->top = frame;
}
void allocatorPopFrame(Allocator* allocator) {
    allocator->arena->allocated = frameRelease(allocator->top) - allocator->arena->start;
    allocator->top = allocator->top->next;
    if (!allocator->top) {
        allocator->top = frameAdd(allocator->arena, NULL);
    }
}

Allocator* allocateChild(Allocator* allocator) {
    Arena* arena = arenaNew(0);
    Allocator* child = poolAdd(allocator->top->children);
    allocatorInit(child, arena);
    return child;
}
void deallocateChild(Allocator* allocator, Allocator* child) {
    poolRemove(allocator->top->children, child);
}

Arena* allocateArena(Allocator* allocator, U64 size) {
    Arena* arena = poolAdd(allocator->top->arenas);
    arenaInit(arena, size);
    return arena;
}
void deallocateArena(Allocator* allocator, Arena* arena) {
    poolRemove(allocator->top->arenas, arena);
}

void* allocate(Allocator* allocator, U64 size) {
    return arenaAlloc(allocator->arena, size);
}

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
        PRINT(CC_MAGENTA, "%s  Frame, allocated: %lu, children: %lu, arenas: %lu\n", indent, size, frame->children->count, frame->arenas->count);  

        PoolNode* node = frame->children->first;
        while (node != NULL) {
            debug((Allocator*)poolData(node), level+4);
            node = node->next;
        }

        node = frame->arenas->first;
        while (node != NULL) {
            PRINT(CC_MAGENTA, "%s    Arena, allocated: %lu\n", indent, ((Arena*)poolData(node))->allocated);
            node = node->next;
        }

        end = start;
        frame = frame->next;
    }
}
void allocatorDebug(Allocator* allocator) {
    debug(allocator, 0);
}