#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/array.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"

Array* arrayNew(Allocator* allocator, U64 item_size, U64 capacity) {
    Array* array = allocate(allocator, sizeof(Array));
    arrayInit(array, allocator->arena, item_size, capacity);
    return array;
}
void arrayInit(Array* array, Arena* arena, U64 item_size, U64 capacity) {
    assert(capacity > 0);

    array->arena = arena;
    array->item_size = item_size;
    array->capacity = capacity;
    array->count = 0;
    array->start = arenaAlloc(array->arena, capacity * item_size);
    array->arena_allocated = array->arena->allocated;
}
void arrayReset(Array* array) {
    array->count = 0;
}

static void* add(Array* array) {
    if (array->count == array->capacity) {
        U64 size = array->capacity * array->item_size;
        if (array->arena_allocated == array->arena->allocated) {
            arenaAlloc(array->arena, size);
        } else {
            U8* new_start = arenaAlloc(array->arena, size * 2);
            memcpy(new_start, array->start, size);
            array->start = new_start;
        }
        array->capacity *= 2;
        array->arena_allocated = array->arena->allocated;
    }

    void* address = array->start + (array->count * array->item_size);
    array->count++;
    return address;
}
void* arrayAdd(Array* array) {
    U8* address = add(array);
    memset(address, 0, array->item_size);
    return address;
}
void* arrayPush(Array* array, const void* item) {
    U8* address = add(array);
    memcpy(address, item, array->item_size);
    return address;
}

void* arrayPop(Array* array) {
    if (array->count > 0) {
        array->count--;
        return array->start + (array->count * array->item_size);
    }
    return NULL;
}

void arraySet(Array* array, U64 index, const void* item) {
    if (index < array->count) {
        U8* address = array->start + (index * array->item_size);
        memcpy(address, item, array->item_size);
    }
}

void* arrayGet(Array* array, U64 index) {
    if (index < array->count) {
        return array->start + (index * array->item_size);
    }
    return NULL;
}

void arrayRemove(Array* array, U64 index) {
    if (index < array->count) {
        array->count--;
        if (index < array->count) {
            U8* address = array->start + (index * array->item_size);
            memmove(address, address + array->item_size, (array->count - index) * array->item_size);
        }
    }
}