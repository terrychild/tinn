#ifndef LIB_MEM_POOL_H
#define LIB_MEM_POOL_H

#include "lib/types.h"

typedef struct PoolNode {
    struct PoolNode* next;
    U8 data[];
} PoolNode;

struct Pool {
    Memory* mem;
    Size node_size;
    Size capacity;
    Size count;
    PoolNode* free;
    PoolNode* first;
};

Pool* poolNew(Memory* mem, Size item_size, Size capacity);
void poolInit(Pool* pool, Memory* mem, Size item_size, Size capacity);
void poolReset(Pool* pool);

void* poolAdd(Pool* pool);
void* poolPush(Pool* pool, const void* item);
void* poolRemove(Pool* pool, const void* item);

void poolDebug(Pool* pool);

#endif