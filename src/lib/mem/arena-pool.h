#ifndef LIB_MEM_ARENA_POOL_H
#define LIB_MEM_ARENA_POOL_H

#include "lib/types.h"
#include "lib/mem/pool.h"

struct ArenaPool {
    Pool pool;
};

void arenaPoolInit(ArenaPool* pool, U64 initial_capacity, U64 max_capacity);
void arenaPoolReset(ArenaPool* pool);
void arenaPoolRelease(ArenaPool* pool);

void arenaPoolAdd(ArenaPool* pool, Arena* arena);
Arena* arenaPoolGet(ArenaPool* pool, U64 index);
void arenaPoolRemove(ArenaPool* pool, Arena* arena);

#endif