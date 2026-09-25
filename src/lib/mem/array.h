#ifndef LIB_MEM_ARRAY_H
#define LIB_MEM_ARRAY_H

#include "lib/types.h"

struct Array {
    Arena* arena;
    U64 item_size;
    U64 capacity;
    U64 count;
    U8* start;
};

Array* arrayNew(Allocator* allocator, U64 item_size, U64 initial_capacity, U64 max_capacity);
void arrayInit(Array* array, Arena* arena, U64 item_size, U64 initial_capacity);
void arrayReset(Array* array);

void* arrayPush(Array* array, const void* item);
void* arrayPop(Array* array);
void arraySet(Array* array, U64 index, const void* item);
void* arrayGet(Array* array, U64 index);
void arrayRemove(Array* array, U64 index);

#endif