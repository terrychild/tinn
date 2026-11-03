#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"
#include "lib/mem/arena.h"
#include "lib/mem/mfile.h"
#include "lib/log.h"

Buffer* bufNew(Memory* mem, U64 size) {
    Buffer* buf = allocate(mem, sizeof(Buffer));
    bufInit(buf, mem, size);
    return buf;
}
void bufInit(Buffer* buf, Memory* mem, U64 size) {
    PANIC_IF(size == 0, "Initial buffer size can not be zero.");

    buf->mem = mem;
    buf->size = size;
    buf->length = 0;
    buf->start = allocate(buf->mem, size);
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
        buf->size = new_size;
        buf->start = reallocate(buf->mem, buf->start, new_size);
    }
}

BufferSpace bufReadyWrite(Buffer* buf, U64 min_size) {
    ensure(buf, min_size);
    return (BufferSpace) {
        .start = buf->start + buf->length,
        .length = buf->size - buf->length
    };
}
void bufConfirmWrite(Buffer* buf, U64 n) {
    PANIC_IF(buf->length + n > buf->size, "Writing beyond the end of the buffer");
    buf->length += n;
}

BufferSlice bufAppendBytes(Buffer* buf, const U8* data, U64 n) {
    ensure(buf, n);
    U64 offset = buf->length;
    buf->length += n;

    memcpy(buf->start + offset, data, n);
    return (BufferSlice) { buf, offset, n};
}
BufferSlice bufAppendSlice(Buffer* buf, const Slice slice) {
    return bufAppendBytes(buf, slice.start, slice.length);
}
BufferSlice bufAppendMappedFile(Buffer* buf, const MappedFile* file) {
    if (file) {
        return bufAppendBytes(buf, file->start, file->length);
    }
    return (BufferSlice) { buf, buf->length, 0};
}
BufferSlice bufAppendStr(Buffer* buf, const char* format, ...) {
    va_list args;
    va_start(args, format);
    U64 n = vsnprintf(NULL, (U64)0, format, args);
    va_end(args);

    ensure(buf, n+1);
    U64 offset = buf->length;
    buf->length += n;

    va_start(args, format);
    vsnprintf((char*)buf->start + offset, n+1, format, args);
    va_end(args);

    return (BufferSlice) { buf, offset, n};
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

U8* bsStart(BufferSlice slice) {
    return slice.buf->start + slice.offset;
}