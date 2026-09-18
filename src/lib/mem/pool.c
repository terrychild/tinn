#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/pool.h"
#include "lib/mem/arena.h"

#include "lib/console.h"

static void extend(Pool* pool, U64 capacity) {
    arenaAlloc(&pool->arena, capacity * pool->node_size);

    U64 index = pool->capacity;
    pool->capacity = pool->arena.allocated / pool->node_size;

    PoolNode* node = (PoolNode*)(pool->data + (index * pool->node_size));
    pool->free = node;
    while (++index < pool->capacity) {
        node->next = (PoolNode*)(pool->data + (index * pool->node_size));
        node = node->next;
    }
    node->next = NULL;    
}

void poolInit(Pool* pool, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* arene_pool) {
    pool->node_size = sizeof(PoolNode*) + item_size;
    
    assert(initial_capacity > 0);
    arenaInit(&pool->arena, max_capacity * pool->node_size, arene_pool);
    assert(pool->arena.size >= initial_capacity * pool->node_size);

    pool->capacity = 0;
    pool->count = 0;
    pool->data = pool->arena.data;
    pool->free = NULL;
    pool->used = NULL;
    extend(pool, initial_capacity);
}
void poolReset(Pool* pool) {
    pool->count = 0;
    pool->free = NULL;
    pool->used = NULL;
}
void poolRelease(Pool* pool) {
    arenaRelease(&pool->arena);
}

void* poolAdd(Pool* pool) {
    if (pool->free == NULL) {
        extend(pool, pool->capacity);
    }

    PoolNode* node = pool->free;
    pool->free = node->next;
    node->next = pool->used;
    pool->used = node;
    pool->count++;

    return node + 1;
}
void* poolPush(Pool* pool, const void* item) {
    void* address = poolAdd(pool);
    memcpy(address, item, pool->node_size - sizeof(PoolNode*));
    return address;
}

void poolRemove(Pool* pool, void* item) {
    if (pool->count > 0 && (U8*)item >= pool->data && (U8*)item < pool->data + (pool->capacity * pool->node_size)) {
        PoolNode* node = pool->used;
        PoolNode* prev = NULL;
        while (node != NULL) {
            if (node + 1 == item) {
                if (prev) {
                    prev->next = node->next;
                } else {
                    pool->used = node->next;
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

void poolDebug(Pool* pool) {
    PRINT(CC_YELLOW, "Pool:\n");

    PRINT(CC_YELLOW, "  Free: ");
    PoolNode* node = pool->free;
    U64 count = 0;
    while (node != NULL && count<32) {
        PRINT(CC_YELLOW, "%lu; ", (((U8*)node) - pool->data) / pool->node_size);
        node = node->next;
        count++;
    }
    PRINT(CC_YELLOW, "NULL\n");
    if (count == 32) {
        PRINT(CC_RED, "over 32\n");
    }

    PRINT(CC_YELLOW, "  Used: ");
    node = pool->used;
    count = 0;
    while (node != NULL && count<32) {
        PRINT(CC_YELLOW, "%lu; ", (((U8*)node) - pool->data) / pool->node_size);
        node = node->next;
        count++;
    }
    if (count == 32) {
        PRINT(CC_RED, "over 32\n");
    }
    PRINT(CC_YELLOW, "NULL\n");
}