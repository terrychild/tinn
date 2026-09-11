#include <stdlib.h>

#include "lib/mem/arena-pool.h"
#include "lib/mem/pool.h"
#include "lib/mem/arena.h"

void arenaPoolInit(ArenaPool* pool, U64 initial_capacity, U64 max_capacity) {
    poolInit(&pool->pool, sizeof(Arena*), initial_capacity ? initial_capacity : 256, max_capacity, NULL);
}
void arenaPoolReset(ArenaPool* pool) {
    for (U64 i=0; i < pool->pool.count; i++) {
        arenaRelease(arenaPoolGet(pool, i));
    }
    poolReset(&pool->pool);
}
void arenaPoolRelease(ArenaPool* pool) {
    arenaPoolReset(pool);
    poolRelease(&pool->pool);
}

void arenaPoolAdd(ArenaPool* pool, Arena* arena) {
    poolAdd(&pool->pool, &arena);
}

Arena* arenaPoolGet(ArenaPool* pool, U64 index) {
    return *(Arena**)poolGet(&pool->pool, index);
}

void arenaPoolRemove(ArenaPool* pool, Arena* arena) {
    for (U64 i=0; i < pool->pool.count; i++) {
        Arena* stored = arenaPoolGet(pool, i);
        if (stored == arena) {
            poolRemove(&pool->pool, i);
            return;
        }
    }
}