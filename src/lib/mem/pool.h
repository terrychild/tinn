#ifndef LIB_MEM_POOL_H
#define LIB_MEM_POOL_H

#include "lib/types.h"

typedef struct PoolNode {
    struct PoolNode* next;
    U8 data[];
} PoolNode;

struct Pool {
    Memory* mem;
    U64 node_size;
    U64 capacity;
    U64 count;
    PoolNode* free;
    PoolNode* first;
};

Pool* poolNew(Memory* mem, U64 item_size, U64 capacity);
void poolInit(Pool* pool, Memory* mem, U64 item_size, U64 capacity);
void poolReset(Pool* pool);

void* poolAdd(Pool* pool);
void* poolPush(Pool* pool, const void* item);
void* poolRemove(Pool* pool, const void* item);

void poolDebug(Pool* pool);

#endif