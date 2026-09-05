#ifndef LIB_MEM_H
#define LIB_MEM_H

#include "lib/types.h"

typedef struct {
    U8* data;
    U64 size;
    U64 committed;
    U64 allocated;
} ArenaAllocator;

void arenaInit(ArenaAllocator* arena, U64 size);
void arenaFree(ArenaAllocator* arena);

void* arenaAlloc(ArenaAllocator* arena, U64 size);
void* arenaAllocZero(ArenaAllocator* arena, U64 size);

void* allocate(void* data, U64 size);

#endif