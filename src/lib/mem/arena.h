#ifndef LIB_MEM_ARENA_H
#define LIB_MEM_ARENA_H

#include "lib/types.h"

struct Arena {
    U64 size;
    U64 committed;
    U64 allocated;
    U8* start;
};

typedef struct {
    U64 size;
    void* start;
} ArenaAllocation;

Arena* arenaNew(U64 size);
void arenaInit(Arena* arean, U64 size);
void arenaReset(Arena* arena);
void arenaRelease(Arena* arena);

ArenaAllocation arenaAllocate(Arena* arena, U64 size);

#endif