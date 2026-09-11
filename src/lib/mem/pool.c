#include <stdlib.h>
#include <string.h>

#include "lib/mem/pool.h"
#include "lib/mem/array.h"
#include "lib/console.h"

void poolInit(Pool* pool, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* arene_pool) {
    arrayInit(&pool->array, item_size, initial_capacity, max_capacity, arene_pool);
    pool->capacity = max_capacity ? max_capacity : pool->array.capacity;
    pool->count = 0;
}
void poolReset(Pool* pool) {
    arrayReset(&pool->array);
    pool->count = 0;
}
void poolRelease(Pool* pool) {
    arrayRelease(&pool->array);
}

void poolAdd(Pool* pool, const void* item) {
    if (pool->count == pool->capacity) {
        PANIC("Pool is out of memory");
    }
    arrayPush(&pool->array, item);
    pool->count++;
}

void* poolGet(Pool* pool, U64 index) {
    if (index >= pool->count) {
        return NULL;
    }
    return arrayGet(&pool->array, index);
}

void poolRemove(Pool* pool, U64 index) {
    if (index < pool->count) {
        pool->count--;
        if (index < pool->count) {
            void* dest = arrayGet(&pool->array, index);
            void* src = arrayGet(&pool->array, pool->count);
            memcpy(dest, src, pool->array.item_size);
        }
        pool->array.count--;
    }
}