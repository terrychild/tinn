#ifndef LIB_MEM_POOL_H
#define LIB_MEM_POOL_H

#include "lib/types.h"
#include "lib/mem/array.h"

struct Pool {
    Array array;
    U64 capacity;
    U64 count;
};

void poolInit(Pool* pool, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* arene_pool);
void poolReset(Pool* pool);
void poolRelease(Pool* pool);

void poolAdd(Pool* pool, const void* item);
void* poolGet(Pool* pool, U64 index);
void poolRemove(Pool* pool, U64 index);

#endif