#ifndef LIB_MEM_ARENA_POOL_H
#define LIB_MEM_ARENA_POOL_H

#include "lib/types.h"
#include "lib/mem/arena.h"

typedef struct ArenaPoolNode ArenaPoolNode;
struct ArenaPoolNode {
    ArenaPoolNode* next;
    Arena arena;
};

struct ArenaPool {
    Arena arena;
    U64 capacity;
    U64 count;
    U8* data;
    ArenaPoolNode* free;
    ArenaPoolNode* used;
};

void arenaPoolInit(ArenaPool* pool, U64 initial_capacity, U64 max_capacity);
void arenaPoolReset(ArenaPool* pool);
void arenaPoolRelease(ArenaPool* pool);

Arena* arenaPoolAdd(ArenaPool* pool, U64 size);
void arenaPoolRemove(ArenaPool* pool, Arena* arena);

#endif