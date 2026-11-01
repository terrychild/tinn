#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/memory.h"
#include "lib/mem/array.h"
#include "lib/mem/arena.h"

Array* arrayNew(Memory* mem, U64 item_size, U64 capacity) {
    Array* array = allocate(mem, sizeof(Array));
    arrayInit(array, mem, item_size, capacity);
    return array;
}
void arrayInit(Array* array, Memory* mem, U64 item_size, U64 capacity) {
    assert(capacity > 0);

    array->mem = mem;
    array->item_size = item_size;
    array->capacity = capacity;
    array->count = 0;
    array->start = allocate(array->mem, capacity * item_size);
}
void arrayReset(Array* array) {
    array->count = 0;
}

static void* add(Array* array) {
    if (array->count == array->capacity) {
        array->capacity *= 2;
        U64 size = array->capacity * array->item_size;
        array->start = reallocate(array->mem, array->start, size);
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

void* arrayPeek(Array* array) {
    if (array->count > 0) {
        return array->start + ((array->count - 1) * array->item_size);
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