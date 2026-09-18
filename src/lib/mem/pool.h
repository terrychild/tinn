#ifndef LIB_MEM_POOL_H
#define LIB_MEM_POOL_H

#include "lib/types.h"

typedef struct PoolNode PoolNode;
struct PoolNode {
    PoolNode* next;
};

struct Pool {
    ArenaPool* arena_pool;
    Arena* arena;
    U64 node_size;
    U64 capacity;
    U64 count;
    PoolNode* free;
    PoolNode* used;
};

void poolInit(Pool* pool, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* arena_pool);
void poolReset(Pool* pool);
void poolRelease(Pool* pool);

void* poolAdd(Pool* pool);
void* poolPush(Pool* pool, const void* item);
void poolRemove(Pool* pool, void* item);

void poolDebug(Pool* pool);

#endif