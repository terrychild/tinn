#ifndef LIB_MEM_H
#define LIB_MEM_H

#include "lib/types.h"

typedef struct {
    u8* data;
    u64 size;
    u64 committed;
    u64 allocated;
} ArenaAllocator;

void arenaInit(ArenaAllocator* arena, u64 size);
void arenaFree(ArenaAllocator* arena);

//void* allocate(void* data, u64 size);

#endif