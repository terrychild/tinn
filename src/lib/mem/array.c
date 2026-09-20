#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/array.h"

Array* arrayNew(Arena* arena, U64 item_size, U64 initial_capacity, U64 max_capacity) {
    Array* array = arenaAlloc(arena, sizeof(Array));
    arrayInit(array, arena, item_size, initial_capacity, max_capacity);
    return array;
}
void arrayInit(Array* array, Arena* arena, U64 item_size, U64 initial_capacity, U64 max_capacity) {
    assert(initial_capacity > 0);
    array->arena = arenaAddChild(arena, max_capacity * item_size, false);
    assert(array->arena->size >= initial_capacity * item_size);

    array->item_size = item_size;
    array->capacity = initial_capacity;
    array->count = 0;
    array->data = arenaAlloc(array->arena, initial_capacity * item_size);
}
void arrayReset(Array* array) {
    array->count = 0;
}

U64 arrayPush(Array* array, const void* item) {
    if (array->count == array->capacity) {
        arenaAlloc(array->arena, array->capacity * array->item_size);
        array->capacity *= 2;
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