#ifndef LIB_MEM_ARRAY_H
#define LIB_MEM_ARRAY_H

#include "lib/types.h"

struct Array {
    Memory* mem;
    Size item_size;
    Size capacity;
    Size count;
    U8* start;
};

Array* arrayNew(Memory* mem, Size item_size, Size capacity);
void arrayInit(Array* array, Memory* mem, Size item_size, Size capacity);
void arrayReset(Array* array);

void* arrayAdd(Array* array);
void* arrayPush(Array* array, const void* item);
void* arrayPop(Array* array);
void* arrayPeek(Array* array);
void arraySet(Array* array, Size index, const void* item);
void* arrayGet(Array* array, Size index);
void arrayRemove(Array* array, Size index);

#endif