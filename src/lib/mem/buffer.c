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

void bufAppendBytes(Buffer* buf, const U8* data, U64 n) {
    ensure(buf, n);

    memcpy(buf->start + buf->length, data, n);
    buf->length += n;
}
void bufAppendSlice(Buffer* buf, const Slice slice) {
    bufAppendBytes(buf, slice.start, slice.length);
}
void bufAppendMappedFile(Buffer* buf, const MappedFile* file) {
    if (file) {
        bufAppendBytes(buf, file->start, file->length);
    }
}
void bufAppendStr(Buffer* buf, const char* format, ...) {
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
    PANIC_IF(buf->length + n > buf->size, "Writing beyond the end of the buffer");
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