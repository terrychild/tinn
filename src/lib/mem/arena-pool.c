#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/arena-pool.h"
#include "lib/mem/arena.h"

#include "lib/console.h"

static const U64 NODE_SIZE = sizeof(ArenaPoolNode);

static void buildFreeList(ArenaPool* pool, U64 index) {
    U8* start = pool->arena.data;
    ArenaPoolNode* node = (ArenaPoolNode*)(start + (index * NODE_SIZE));
    pool->free = node;
    while (++index < pool->capacity) {
        node->next = (ArenaPoolNode*)(start + (index * NODE_SIZE));
        node = node->next;
    }
    node->next = NULL;
}
static void extend(ArenaPool* pool, U64 capacity) {
    arenaAlloc(&pool->arena, capacity * NODE_SIZE);

    U64 index = pool->capacity;
    pool->capacity = pool->arena.allocated / NODE_SIZE;
    buildFreeList(pool, index);    
}

void arenaPoolInit(ArenaPool* pool, U64 initial_capacity, U64 max_capacity) {
    initial_capacity = initial_capacity ? initial_capacity : 8;
    arenaInit(&pool->arena, max_capacity * NODE_SIZE);
    assert(pool->arena.size >= initial_capacity * NODE_SIZE);

    pool->capacity = 0;
    pool->count = 0;
    pool->free = NULL;
    pool->used = NULL;
    extend(pool, initial_capacity);
}
void arenaPoolReset(ArenaPool* pool) {
    ArenaPoolNode* node = pool->used;
    while (node != NULL) {
        arenaRelease(&node->arena);
        node = node->next;
    }

    pool->count = 0;
    buildFreeList(pool, 0);
    pool->used = NULL;
}
void arenaPoolRelease(ArenaPool* pool) {
    arenaPoolReset(pool);
    arenaRelease(&pool->arena);
}

Arena* arenaPoolAdd(ArenaPool* pool, U64 size) {
    if (pool->free == NULL) {
        extend(pool, pool->capacity);
    }

    ArenaPoolNode* node = pool->free;
    pool->free = node->next;
    node->next = pool->used;
    pool->used = node;
    pool->count++;

    arenaInit(&node->arena, size);
    return &node->arena;
}

void arenaPoolRemove(ArenaPool* pool, Arena* arena) {
    U8* start = pool->arena.data;
    U8* end = start + (pool->capacity * NODE_SIZE);
    U8* address = (U8*)arena;
    if (pool->count > 0 && address >= start && address < end) {
        ArenaPoolNode* node = pool->used;
        ArenaPoolNode* prev = NULL;
        while (node != NULL) {
            if (arena == &node->arena) {
                arenaRelease(arena);
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