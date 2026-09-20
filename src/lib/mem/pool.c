#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/pool.h"
#include "lib/mem/arena.h"
#include "lib/log.h"
#include "lib/cli.h"

static void buildFreeList(Pool* pool, U64 index) {
    PoolNode* node = (PoolNode*)(pool->start + (index * pool->node_size));
    pool->free = node;
    while (++index < pool->capacity) {
        node->next = (PoolNode*)(pool->start + (index * pool->node_size));
        node = node->next;
    }
    node->next = NULL;
}

Pool* poolNew(Arena* arena, U64 item_size, U64 initial_capacity, U64 max_capacity) {
    Pool* pool = arenaAlloc(arena, sizeof(Pool));
    poolInit(pool, arena, item_size, initial_capacity, max_capacity);
    return pool;
}
void poolInit(Pool* pool, Arena* arena, U64 item_size, U64 initial_capacity, U64 max_capacity) {
    pool->node_size = sizeof(PoolNode*) + item_size;
    
    assert(initial_capacity > 0);
    pool->arena = arenaAddChild(arena, max_capacity * pool->node_size, false);
    assert(pool->arena->size >= initial_capacity * pool->node_size);

    pool->capacity = initial_capacity;
    pool->count = 0;
    pool->start = arenaAlloc(pool->arena, initial_capacity * pool->node_size);
    buildFreeList(pool, 0);
    pool->first = NULL;    
}
void poolReset(Pool* pool) {
    pool->count = 0;
    buildFreeList(pool, 0);
    pool->first = NULL;
}

void* poolAdd(Pool* pool) {
    if (pool->free == NULL) {
        arenaAlloc(pool->arena, pool->capacity * pool->node_size);
        pool->capacity *= 2;
        buildFreeList(pool, pool->count);
    }

    PoolNode* node = pool->free;
    pool->free = node->next;
    node->next = pool->first;
    pool->first = node;
    pool->count++;

    return poolData(node);
}
void* poolPush(Pool* pool, const void* item) {
    void* address = poolAdd(pool);
    memcpy(address, item, pool->node_size - sizeof(PoolNode*));
    return address;
}
void poolRemove(Pool* pool, void* item) {
    U8* end = pool->start + (pool->capacity * pool->node_size);
    U8* address = (U8*)item;
    if (pool->count > 0 && address >= pool->start && address < end) {
        PoolNode* node = pool->first;
        PoolNode* prev = NULL;
        while (node != NULL) {
            if (poolData(node) == item) {
                if (prev) {
                    prev->next = node->next;
                } else {
                    pool->first = node->next;
                }
                node->next = pool->free;
                pool->free = node;
                pool->count--;
                return;
            }
            prev = node;
            node = node->next;
        }
    }
}

void* poolData(PoolNode* node) {
    return ((U8*)node) + sizeof(PoolNode*);
}

void poolDebug(Pool* pool) {
    PRINT(CC_YELLOW, "Pool:\n");

    PRINT(CC_YELLOW, "  Free: ");
    PoolNode* node = pool->free;
    U64 count = 0;
    while (node != NULL && count<32) {
        PRINT(CC_YELLOW, "%lu; ", (((U8*)node) - pool->start) / pool->node_size);
        node = node->next;
        count++;
    }
    PRINT(CC_YELLOW, "NULL\n");
    if (count == 32) {
        PRINT(CC_RED, "over 32\n");
    }

    PRINT(CC_YELLOW, "  Used: ");
    node = pool->first;
    count = 0;
    while (node != NULL && count<32) {
        PRINT(CC_YELLOW, "%lu; ", (((U8*)node) - pool->start) / pool->node_size);
        node = node->next;
        count++;
    }
    if (count == 32) {
        PRINT(CC_RED, "over 32\n");
    }
    PRINT(CC_YELLOW, "NULL\n");
}