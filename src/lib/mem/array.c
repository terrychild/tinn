#include <stdlib.h>
#include <string.h>

#include "lib/mem/array.h"
#include "lib/mem/arena.h"

void arrayInit(Array* array, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* pool) {
    arenaInit(&array->arena, max_capacity * item_size, pool);
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

void arrayPush(Array* array, const void* item) {
    if (array->count == array->capacity) {
        arenaAlloc(&array->arena, array->capacity * array->item_size);
        array->capacity = array->arena.allocated / array->item_size;
    }

    U8* address = array->data + (array->count * array->item_size);
    memcpy(address, item, array->item_size);
    array->count++;
}

void* arrayGet(Array* array, U64 index) {
    if (index >= array->count) {
        return NULL;
    }
    return array->data + (index * array->item_size);
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