#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/array.h"
#include "lib/mem/arena-pool.h"

static void extend(Array* array, U64 capacity) {
    arenaAlloc(array->arena, capacity * array->item_size);
    array->capacity = array->arena->allocated / array->item_size;
}

void arrayInit(Array* array, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* arena_pool) {
    assert(initial_capacity > 0);
    array->arena_pool = arena_pool;
    array->arena = arenaPoolAdd(arena_pool, max_capacity * item_size);
    assert(array->arena->size >= initial_capacity * item_size);

    array->item_size = item_size;
    array->capacity = 0;
    array->count = 0;
    array->data = array->arena->data;
    extend(array, initial_capacity);
}
void arrayReset(Array* array) {
    array->count = 0;
}
void arrayRelease(Array* array) {
    arenaPoolRemove(array->arena_pool, array->arena);
}

U64 arrayPush(Array* array, const void* item) {
    if (array->count == array->capacity) {
        extend(array, array->capacity);
    }

    U8* address = array->data + (array->count * array->item_size);
    memcpy(address, item, array->item_size);
    array->count++;
    return array->count;
}

void* arrayPop(Array* array) {
    if (array->count > 0) {
        array->count--;
        return array->data + (array->count * array->item_size);
    }
    return NULL;
}

void arraySet(Array* array, U64 index, const void* item) {
    if (index < array->count) {
        U8* address = array->data + (index * array->item_size);
        memcpy(address, item, array->item_size);
    }
}

void* arrayGet(Array* array, U64 index) {
    if (index < array->count) {
        return array->data + (index * array->item_size);
    }
    return NULL;
}

void arrayRemove(Array* array, U64 index) {
    if (index < array->count) {
        array->count--;
        if (index < array->count) {
            U8* address = array->data + (index * array->item_size);
            memmove(address, address + array->item_size, (array->count - index) * array->item_size);
        }
    }
}