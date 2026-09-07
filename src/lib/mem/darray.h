#ifndef LIB_MEM_DARRAY_H
#define LIB_MEM_DARRAY_H

#include "lib/types.h"
#include "lib/mem/arena.h"

typedef struct {
    ArenaAllocator arena;
    U8* data;
    U64 item_size;
    U64 capacity;
    U64 count;  
} DynamicArray;

void daInit(DynamicArray* da, U64 item_size, U64 initial_capacity, U64 max_capacity);
void daReset(DynamicArray* da);
void daRelease(DynamicArray* da);

void daPush(DynamicArray* da, const void* item);
void* daGet(DynamicArray* da, U64 index);

#endif