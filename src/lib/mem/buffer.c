#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "lib/mem/buffer.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"

Buffer* bufNew(Allocator* allocator, U64 size) {
    Buffer* buf = allocate(allocator, sizeof(Buffer));
    bufInit(buf, allocator->arena, size);
    return buf;
}
void bufInit(Buffer* buf, Arena* arena, U64 size) {
    assert(size > 0);

    buf->arena = arena;
    buf->size = size;
    buf->length = 0;
    buf->start = arenaAlloc(buf->arena, size);
    buf->arena_allocated = buf->arena->allocated;
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
        if (buf->arena_allocated == buf->arena->allocated) {
            arenaAlloc(buf->arena, new_size - buf->size);
        } else {
            U8* new_start = arenaAlloc(buf->arena, new_size);
            memcpy(new_start, buf->start, buf->length);
            buf->start = new_start;
        }
        buf->size = new_size;
        buf->arena_allocated = buf->arena->allocated;
    }
}

void bufAppend(Buffer* buf, const U8* data, U64 n) {
    ensure(buf, n);

    memcpy(buf->start + buf->length, data, n);
    buf->length += n;
}
void bufAppendSlice(Buffer* buf, const Slice slice) {
    bufAppend(buf, slice.start, slice.length);
}
void bufAppendStr(Buffer* buf, const char* str) {
    bufAppend(buf, (U8*)str, strlen(str));
}
void bufAppendFormat(Buffer* buf, const char* format, ...) {
    va_list args;
    va_start(args, format);
    U64 n = vsnprintf(NULL, (U64)0, format, args);
    va_end(args);

    ensure(buf, n+1);

    va_start(args, format);
    vsnprintf((char*)buf->start + buf->length, n+1, format, args);
    va_end(args);

    buf->length += n;
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

Slice bufSlice(Buffer* buf, U64 start, U64 end) {
    if (start > buf->length) {
        start = buf->length;
    }
    if (end > buf->length) {
        end = buf->length;
    }
    return (Slice) {
        .length = end - start,
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