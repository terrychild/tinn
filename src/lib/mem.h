#ifndef LIB_MEM_H
#define LIB_MEM_H

#include "lib/types.h"

typedef struct {
    U8* data;
    U64 size;
    U64 committed;
    U64 allocated;
} ArenaAllocator;

ArenaAllocator* arenaNew(U64 arena_size);
void arenaReset(ArenaAllocator* arena);
void arenaRelease(ArenaAllocator* arena);

void* arenaAlloc(ArenaAllocator* arena, U64 size);
void* arenaAllocRaw(ArenaAllocator* arena, U64 size);

void* allocate(void* data, U64 size);

#endif