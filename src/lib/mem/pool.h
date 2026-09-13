#ifndef LIB_MEM_POOL_H
#define LIB_MEM_POOL_H

#include "lib/types.h"
#include "lib/mem/array.h"

typedef struct PoolSlot PoolSlot;

struct PoolSlot {
    PoolSlot* next;
};

struct Pool {
    Arena arena;
    U8* data;
    PoolSlot* free;
    U64 item_size;
    U64 capacity;
    U64 count;
};

void poolInit(Pool* pool, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* arene_pool);
void poolReset(Pool* pool);
void poolRelease(Pool* pool);

void* poolAdd(Pool* pool);
void* poolPush(Pool* pool, const void* item);
void poolRemove(Pool* pool, void* item);

void poolDebug(Pool* pool);

#endif