#ifndef LIB_D_STRING
#define LIB_D_STRING

#include "lib/types.h"

struct String {
    ArenaPool* arena_pool;
    Arena* arena;
    U64 alloc_size;
    U64 length;
    U8* data;    
};

void stringInit(String* string, ArenaPool* arena_pool, U64 alloc_size, U64 max_size);
void stringReset(String* string);
void stringRelease(String* string);

void stringExtend(String* string);

// todo: slice!

U8* stringWritePtr(String* string);
U64 stringWriteMax(String* string);

void stringAppend(String* string, const U8* data, U64 n);

U8* stringReserve(String* string, U64 n);

#endif