#include <string.h>
#include <assert.h>

#include "lib/macros.h"
#include "lib/d-string.h"
#include "lib/mem/arena-pool.h"

void ensure(String* string, U64 n) {
    U64 new_length = string->length + n;
    U64 new_size = string->arena->allocated;
    while (new_size < new_length) {
        new_size += string->alloc_size;
    }    
    if (new_size > string->arena->allocated) {
        arenaAlloc(string->arena, new_size - string->arena->allocated);
    }
}

void stringInit(String* string, ArenaPool* arena_pool, U64 alloc_size, U64 max_size) {
    assert(max_size == 0 || max_size >= alloc_size);
    string->arena_pool = arena_pool;
    string->arena = arenaPoolAdd(arena_pool, max_size);

    string->alloc_size = alloc_size ? alloc_size : KB(4);
    string->length = 0;
    string->data = string->arena->data;
    ensure(string, alloc_size);
}
void stringReset(String* string) {
    string->length = 0;
    //TODO: blank string?
}
void stringRelease(String* string) {
    arenaPoolRemove(string->arena_pool, string->arena);
}

void stringExtend(String* string) {
    ensure(string, string->alloc_size);
}

U8* stringWritePtr(String* string) {
    return string->data + string->length;
}
U64 stringWriteMax(String* string) {
    return string->arena->allocated - string->length;
}

void stringAppend(String* string, const U8* data, U64 n) {
    ensure(string, n);
    memcpy(stringWritePtr(string), data, n);
    string->length += n;
}

U8* stringReserve(String* string, U64 n) {
    ensure(string, n);
    U8* rv = stringWritePtr(string);
    string->length += n;
    return rv;
}