#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/buffer.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"

Buffer* bufNew(Allocator* allocator, U64 initial_size, U64 max_size) {
    Buffer* buf = allocate(allocator, sizeof(Buffer));
    bufInit(buf, allocateArena(allocator, max_size), initial_size);
    return buf;
}
void bufInit(Buffer* buf, Arena* arena, U64 initial_size) {
    assert(initial_size > 0);
    assert(arena->size >= initial_size);

    buf->arena = arena;
    buf->size = initial_size;
    buf->length = 0;
    buf->start = arenaAlloc(buf->arena, initial_size);
}
void bufReset(Buffer* buf) {
    buf->length = 0;
}

static void ensure(Buffer* buf, U64 n) {
    U64 new_size = buf->size;
    while (new_size < buf->length + n) {
        new_size *= 2;
    }
    if (new_size > buf->size) {
        arenaAlloc(buf->arena, new_size - buf->size);
        buf->size = new_size;
    }
}

void bufAppend(Buffer* buf, const U8* data, U64 n) {
    ensure(buf, n);

    memcpy(buf->start + buf->length, data, n);
    buf->length += n;
}
void bufAppendStr(Buffer* buf, const char* str) {
    bufAppend(buf, (U8*)str, strlen(str));  
}

BufferSpace bufReadyWrite(Buffer* buf, U64 min_size) {
    ensure(buf, min_size);
    return (BufferSpace) {
        .length = buf->size - buf->length,
        .start = buf->start + buf->length
    };
}
void bufConfirmWrite(Buffer* buf, U64 n) {
    assert(buf->length + n <= buf->size);
    buf->length += n;
}

Slice bufSlice(Buffer* buf, U64 start, U64 length) {
    if (start > buf->size) {
        start = buf->size;
    }
    if (length == 0) {
        length = (start < buf->length ? buf->length : buf->size) - start;
    } else {
        if (start + length > buf->size) {
            length = buf->size - start;
        }
    }
    return (Slice) {
        .length = length,
        .start = buf->start + start
    };
}
Slice bufAsSlice(Buffer* buf) {
    return (Slice) {
        .length = buf->length,
        .start = buf->start
    };
}

char* bufAsStr(Buffer* buf) {
    ensure(buf, 1);
    buf->start[buf->length] = '\0';
    return (char*)buf->start;
}