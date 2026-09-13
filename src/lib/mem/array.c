#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/array.h"
#include "lib/mem/arena.h"

void arrayInit(Array* array, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* pool) {
    assert(initial_capacity > 0);

    arenaInit(&array->arena, max_capacity * item_size, pool);

    assert(array->arena.size >= initial_capacity * item_size);

    array->data = arenaAlloc(&array->arena, initial_capacity * item_size);
    array->item_size = item_size;
    array->capacity = array->arena.allocated / item_size;
    array->count = 0;
}
void arrayReset(Array* array) {
    array->count = 0;
}
void arrayRelease(Array* array) {
    arenaRelease(&array->arena);
}

U64 arrayPush(Array* array, const void* item) {
    if (array->count == array->capacity) {
        arenaAlloc(&array->arena, array->capacity * array->item_size);
        array->capacity = array->arena.allocated / array->item_size;
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