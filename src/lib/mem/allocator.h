#ifndef LIB_MEM_ALLOCATOR_H
#define LIB_MEM_ALLOCATOR_H

#include <lib/types.h>

typedef struct AllocatorFrame {
    struct AllocatorFrame* next;
    Pool* children;
    Pool* arenas;
} AllocatorFrame;

struct Allocator {
    Arena* arena;
    AllocatorFrame* top;
};

Allocator* allocatorNew();
void allocatorInit(Allocator* allocator, Arena* arena);
void allocatorReset(Allocator* allocator);
void allocatorRelease(Allocator* allocator);

void allocatorPushFrame(Allocator* allocator);
void allocatorPopFrame(Allocator* allocator);

Allocator* allocateChild(Allocator* allocator);
void deallocateChild(Allocator* allocator, Allocator* child);

Arena* allocateArena(Allocator* allocator, U64 size);
void deallocateArena(Allocator* allocator, Arena* arena);

void* allocate(Allocator* allocator, U64 size);

void allocatorDebug(Allocator* allocator);

#endif