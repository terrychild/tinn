#ifndef LIB_MEM_POOL_H
#define LIB_MEM_POOL_H

#include "lib/types.h"
#include "lib/mem/array.h"

typedef struct {
    Array array;
    U64 capacity;
    U64 count;
} Pool;

void poolInit(Pool* pool, U64 item_size, U64 initial_capacity, U64 max_capacity);
void poolReset(Pool* pool);
void poolRelease(Pool* pool);

void poolAdd(Pool* pool, const void* item);
void* poolGet(Pool* pool, U64 index);
void poolRemove(Pool* pool, U64 index);

#endif