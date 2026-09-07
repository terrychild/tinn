#include <stdlib.h>
#include <string.h>

#include "lib/mem/arena.h"
#include "lib/mem/darray.h"

void daInit(DynamicArray* da, U64 item_size, U64 initial_capacity, U64 max_capacity) {
    arenaInit(&da->arena, max_capacity * item_size);
    da->data = arenaAlloc(&da->arena, initial_capacity * item_size);
    da->item_size = item_size;
    da->capacity = da->arena.allocated / item_size;
    da->count = 0;
}
void daReset(DynamicArray* da) {
    da->count = 0;
}
void daRelease(DynamicArray* da) {
    arenaRelease(&da->arena);
}

void daPush(DynamicArray* da, const void* item) {
    if (da->count == da->capacity) {
        arenaAlloc(&da->arena, da->capacity * da->item_size);
        da->capacity = da->arena.allocated / da->item_size;
    }

    void* address = da->data + (da->count * da->item_size);
    memcpy(address, item, da->item_size);
    da->count++;
}

void* daGet(DynamicArray* da, U64 index) {
    if (index >= da->count) {
        return NULL;
    }
    return da->data + (index * da->item_size);
}