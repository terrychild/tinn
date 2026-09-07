#ifndef LIB_MEM_ARENA_H
#define LIB_MEM_ARENA_H

#include "lib/types.h"

typedef struct {
    U8* data;
    U64 size;
    U64 committed;
    U64 allocated;
} ArenaAllocator;

void arenaInit(ArenaAllocator* arean, U64 size);
void arenaReset(ArenaAllocator* arena);
void arenaRelease(ArenaAllocator* arena);

void* arenaAlloc(ArenaAllocator* arena, U64 size);
void* arenaAllocRaw(ArenaAllocator* arena, U64 size);

#endif