#ifndef LIB_MEM_ARENA_H
#define LIB_MEM_ARENA_H

#include "lib/types.h"

struct Arena {
    Size size;
    Size committed;
    Size allocated;
    U8* start;
};

typedef struct {
    Size size;
    void* start;
} ArenaAllocation;

Arena* arenaNew(Size size);
void arenaInit(Arena* arean, Size size);
void arenaReset(Arena* arena);
void arenaRelease(Arena* arena);

ArenaAllocation arenaAllocate(Arena* arena, Size size);

#endif