#ifndef LIB_MEM_ARRAY_H
#define LIB_MEM_ARRAY_H

#include "lib/types.h"
#include "lib/mem/arena.h"

struct Array {
    Arena arena;
    U64 item_size;
    U64 capacity;
    U64 count;
    U8* data;
};

void arrayInit(Array* array, U64 item_size, U64 initial_capacity, U64 max_capacity, ArenaPool* pool);
void arrayReset(Array* array);
void arrayRelease(Array* array);

U64 arrayPush(Array* array, const void* item);
void* arrayPop(Array* array);
void arraySet(Array* array, U64 index, const void* item);
void* arrayGet(Array* array, U64 index);
void arrayRemove(Array* array, U64 index);

#endif