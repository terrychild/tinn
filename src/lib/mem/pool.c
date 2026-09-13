#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/pool.h"
#include "lib/mem/arena.h"

#include "lib/console.h"

void poolInit(Pool* pool, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* arene_pool) {
    assert(item_size >= sizeof(PoolSlot*));
    assert(initial_capacity > 0);

    arenaInit(&pool->arena, max_capacity * item_size, arene_pool);

    assert(pool->arena.size >= initial_capacity * item_size);

    pool->data = arenaAlloc(&pool->arena, initial_capacity * item_size);
    pool->free = NULL;
    pool->item_size = item_size;
    pool->capacity = pool->arena.allocated / item_size;
    pool->count = 0;
}
void poolReset(Pool* pool) {
    pool->free = NULL;
    pool->count = 0;
}
void poolRelease(Pool* pool) {
    arenaRelease(&pool->arena);
}

void* poolAdd(Pool* pool) {
    /*if (pool->free != NULL) {
        void* address = pool->free;
        pool->free = (U8*)pool->free;
        pool->count++;
        return address;
    }*/

    if (pool->count == pool->capacity) {
        arenaAlloc(&pool->arena, pool->capacity * pool->item_size);
        pool->capacity = pool->arena.allocated / pool->item_size;
    }

    U8* address = pool->data + (pool->count * pool->item_size);
    pool->count++;
    return address;
}
void* poolPush(Pool* pool, const void* item) {
    void* address = poolAdd(pool);
    memcpy(address, item, pool->item_size);
    return address;
}

void poolRemove(Pool* pool, void* item) {
    //TODO: check the address is in the correct range
    if (pool->count > 0) {
        pool->count--;

        *(PoolSlot *)item = (PoolSlot) {
            .next = pool->free
        };
        pool->free= item;
    }
}

void poolDebug(Pool* pool) {
    PRINT(CC_YELLOW, "Pool:\n");
    PRINT(CC_YELLOW, "  Free: ");
    PoolSlot* chunk = pool->free;
    while (chunk != NULL) {
        PRINT(CC_YELLOW, "%lu; ", (((U8*)chunk) - pool->data) / pool->item_size);
        chunk = chunk->next;
    } while (chunk);
    PRINT(CC_YELLOW, "NULL\n");
}