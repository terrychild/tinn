#ifndef LIB_MEM_ARRAY_H
#define LIB_MEM_ARRAY_H

#include "lib/types.h"
#include "lib/mem/arena.h"

typedef struct {
    ArenaAllocator arena;
    U8* data;
    U64 item_size;
    U64 capacity;
    U64 count;
} Array;

void arrayInit(Array* array, U64 item_size, U64 initial_capacity, U64 max_capacity);
void arrayReset(Array* array);
void arrayRelease(Array* array);

void arrayPush(Array* array, const void* item);
void* arrayGet(Array* array, U64 index);

#endif