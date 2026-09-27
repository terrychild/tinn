#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/pool.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"
#include "lib/log.h"
#include "lib/cli.h"

static void buildFreeList(Pool* pool, U8* start, U64 count) {
    PoolNode* node = (PoolNode*)start;
    pool->free = node;
    for (U64 index = 1; index < count; index++) {
        node->next = (PoolNode*)(start + (index * pool->node_size));
        node = node->next;
    }
    node->next = NULL;
}

Pool* poolNew(Allocator* allocator, U64 item_size, U64 capacity) {
    Pool* pool = allocate(allocator, sizeof(Pool));
    poolInit(pool, allocator->arena, item_size, capacity);
    return pool;
}
void poolInit(Pool* pool, Arena* arena, U64 item_size, U64 capacity) {
    assert(capacity > 0);

    pool->arena = arena;
    pool->node_size = sizeof(PoolNode*) + item_size;
    pool->capacity = capacity;
    pool->count = 0;
    buildFreeList(pool, arenaAlloc(pool->arena, capacity * pool->node_size), capacity);
    pool->first = NULL;
}

void poolReset(Pool* pool) {
    PoolNode* node = pool->first;
    while (node->next != NULL) {
        node = node->next;
    }
    node->next = pool->free;
    pool->free = pool->first;
    pool->first = NULL;
    pool->count = 0;
}

void* poolAdd(Pool* pool) {
    if (pool->free == NULL) {
        buildFreeList(pool, arenaAlloc(pool->arena, pool->capacity * pool->node_size), pool->capacity);
        pool->capacity *= 2;
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

void poolRemove(Pool* pool, const void* item) {
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

void* poolData(PoolNode* node) {
    return ((U8*)node) + sizeof(PoolNode*);
}

void poolDebug(Pool* pool) {
    PRINT(CC_YELLOW, "Pool:\n");

    PRINT(CC_YELLOW, "  Free: ");
    PoolNode* node = pool->free;
    U64 count = 0;
    while (node != NULL && count<32) {
        PRINT(CC_YELLOW, "%lu; ", ((U8*)node) - pool->arena->start);
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
        PRINT(CC_YELLOW, "%lu; ", ((U8*)node) - pool->arena->start);
        node = node->next;
        count++;
    }
    if (count == 32) {
        PRINT(CC_RED, "over 32\n");
    }
    PRINT(CC_YELLOW, "NULL\n");
}