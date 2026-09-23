#include <stdlib.h>
#include <string.h>

#include "lib/mem/allocator.h"
#include <lib/mem/arena.h>
#include <lib/mem/pool.h>

#define CHILDREN_PER_FRAME 16
#define ARENAS_PER_FRAME 16

// frames
static AllocatorFrame* frameAdd(Allocator* allocator, AllocatorFrame* next) {
    AllocatorFrame* frame = arenaAlloc(&allocator->arena, sizeof(AllocatorFrame));
    frame->next = next;

    Arena* children_arena = arenaAlloc(&allocator->arena, sizeof(Arena));
    arenaInit(children_arena, 0);
    frame->children = arenaAlloc(&allocator->arena, sizeof(Pool));
    poolInit(frame->children, children_arena, sizeof(Allocator), CHILDREN_PER_FRAME);

    Arena* arenas_arena = arenaAlloc(&allocator->arena, sizeof(Arena));
    arenaInit(arenas_arena, 0);
    frame->arenas = arenaAlloc(&allocator->arena, sizeof(Pool));
    poolInit(frame->arenas, arenas_arena, sizeof(Arena), ARENAS_PER_FRAME);

    return frame;
}
static void frameRelease(AllocatorFrame* frame) {
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
}

// Allocator
Allocator* allocatorNew() {
    Allocator temp_allocator;
    allocatorInit(&temp_allocator);
    Allocator* allocator = allocate(&temp_allocator, sizeof(Allocator));
    memcpy(allocator, &temp_allocator, sizeof(Allocator));
    return allocator;
}
void allocatorInit(Allocator* allocator) {
    arenaInit(&allocator->arena, 0);
    allocator->top = frameAdd(allocator, NULL);
    allocator->inital_allocated = allocator->arena.allocated;
}
void allocatorReset(Allocator* allocator) {
    AllocatorFrame* frame = allocator->top;
    while (frame) {
        frameRelease(frame);
        frame = frame->next;
    }
}
void allocatorRelease(Allocator* allocator) {
    allocatorReset(allocator);
    arenaRelease(&allocator->arena);
}

void allocatorPushFrame(Allocator* allocator) {
    AllocatorFrame* frame = frameAdd(allocator, allocator->top);
    allocator->top = frame;
}
void allocatorPopFrame(Allocator* allocator) {
    frameRelease(allocator->top);

    if (allocator->top->next) {
        allocator->arena.allocated = (U8*)allocator->top - allocator->arena.start;
        allocator->top = allocator->top->next;
    } else {
        poolReset(allocator->top->children);
        poolReset(allocator->top->arenas);
        allocator->arena.allocated = allocator->inital_allocated;
    }
}

Allocator* allocateChild(Allocator* allocator) {
    Allocator* child = poolAdd(allocator->top->children);
    allocatorInit(child);
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
    return arenaAlloc(&allocator->arena, size);
}
