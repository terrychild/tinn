#ifndef LIB_MEM_ARENA_H
#define LIB_MEM_ARENA_H

#include "lib/types.h"

struct Arena {
    U8* data;
    U64 size;
    U64 committed;
    U64 allocated;
    ArenaPool* pool;
};

void arenaInit(Arena* arean, U64 size, ArenaPool* pool);
void arenaReset(Arena* arena);
void arenaRelease(Arena* arena);

void* arenaAlloc(Arena* arena, U64 size);
void* arenaAllocRaw(Arena* arena, U64 size);

#endif