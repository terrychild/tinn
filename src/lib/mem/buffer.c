#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"
#include "lib/mem/arena.h"
#include "lib/mem/mfile.h"
#include "lib/bytes.h"
#include "lib/log.h"

Buffer* bufNew(Memory* mem, Size size) {
    Buffer* buf = allocate(mem, sizeof(Buffer));
    bufInit(buf, mem, size);
    return buf;
}
void bufInit(Buffer* buf, Memory* mem, Size size) {
    PANIC_IF(size == 0, "Initial buffer size can not be zero.");

    buf->mem = mem;
    buf->size = size;
    buf->length = 0;
    buf->start = allocate(buf->mem, size);
}
void bufReset(Buffer* buf) {
    buf->length = 0;
}

static void ensure(Buffer* buf, Size n) {
    Size new_size = buf->size;
    while (new_size < buf->length + n) {
        new_size *= 2;
    }
    if (new_size > buf->size) {
        buf->size = new_size;
        buf->start = reallocate(buf->mem, buf->start, new_size);
    }
}

BufferSpace bufReadyWrite(Buffer* buf, Size min_size) {
    ensure(buf, min_size);
    return (BufferSpace) {
        .start = buf->start + buf->length,
        .length = buf->size - buf->length
    };
}
void bufConfirmWrite(Buffer* buf, Size n) {
    PANIC_IF(buf->length + n > buf->size, "Writing beyond the end of the buffer");
    buf->length += n;
}

Size bufReserve(Buffer* buf, Size n) {
    ensure(buf, n);
    Size offset = buf->length;
    buf->length += n;
    return offset;
}
void bufConsume(Buffer* buf, Size n) {
    if (n == buf->length) {
        bufReset(buf);
    } else if (n < buf->length) {
        Size new_len = buf->length - n;
        memmove(buf->start, buf->start + n, new_len);
        buf->length = new_len;
    }
}

void bufAppendBytes(Buffer* buf, const U8* data, Size n) {
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
    Size n = vsnprintf(NULL, (Size)0, format, args);
    va_end(args);

    ensure(buf, n+1);

    va_start(args, format);
    vsnprintf((char*)buf->start + buf->length, n+1, format, args);
    va_end(args);

    buf->length += n;
}

void bufAppendHex(Buffer* buf, const char* hex) {
    Size nibbles = strlen(hex);
    Size len = (nibbles + 1) / 2;
    ensure(buf, len);

    readHexStr(buf->start + buf->length, nibbles, hex);
    buf->length += len;
}

void bufAppendBE8(Buffer* buf, U8 i) {
    ensure(buf, 1);
    toBE8(buf->start + buf->length, i);
    buf->length += 1;
}
void bufAppendBE16(Buffer* buf, U16 i) {
    ensure(buf, 1);
    toBE16(buf->start + buf->length, i);
    buf->length += 2;
}
void bufAppendBE24(Buffer* buf, U32 i) {
    ensure(buf, 1);
    toBE24(buf->start + buf->length, i);
    buf->length += 3;
}
void bufAppendBE32(Buffer* buf, U32 i) {
    ensure(buf, 1);
    toBE32(buf->start + buf->length, i);
    buf->length += 4;
}
void bufAppendBE64(Buffer* buf, U64 i) {
    ensure(buf, 1);
    toBE64(buf->start + buf->length, i);
    buf->length += 8;
}

void bufSetBE8(Buffer* buf, Size offset, U8 i) {
    PANIC_IF(offset + 1 > buf->length, "Writing beyond the end of the buffer");
    toBE8(buf->start + offset, i);
}
void bufSetBE16(Buffer* buf, Size offset, U16 i) {
    PANIC_IF(offset + 2 > buf->length, "Writing beyond the end of the buffer");
    toBE16(buf->start + offset, i);
}
void bufSetBE24(Buffer* buf, Size offset, U32 i) {
    PANIC_IF(offset + 3 > buf->length, "Writing beyond the end of the buffer");
    toBE24(buf->start + offset, i);
}
void bufSetBE32(Buffer* buf, Size offset, U32 i) {
    PANIC_IF(offset + 4 > buf->length, "Writing beyond the end of the buffer");
    toBE32(buf->start + offset, i);
}
void bufSetBE64(Buffer* buf, Size offset, U64 i) {
    PANIC_IF(offset + 8 > buf->length, "Writing beyond the end of the buffer");
    toBE64(buf->start + offset, i);
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