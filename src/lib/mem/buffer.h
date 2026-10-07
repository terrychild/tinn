#ifndef LIB_MEM_BUFFER_H
#define LIB_MEM_BUFFER_H

#include "lib/types.h"

struct Buffer {
    Arena* arena;
    U64 arena_allocated;
    U64 size;
    U64 length;
    U8* start;
};

typedef struct {
    U64 length;
    U8* start;
} BufferSpace;

Buffer* bufNew(Allocator* allocator, U64 size);
void bufInit(Buffer* buf, Arena* arena, U64 size);
void bufReset(Buffer* buf);

void bufAppend(Buffer* buf, const U8* data, U64 n);
void bufAppendSlice(Buffer* buf, const Slice slice);
void bufAppendMFile(Buffer* buf, MappedFile* file);
void bufAppendStr(Buffer* buf, const char* str);
void bufAppendFormat(Buffer* buf, const char* format, ...);

BufferSpace bufReadyWrite(Buffer* buf, U64 min_size);
void bufConfirmWrite(Buffer* buf, U64 n);

Slice bufSlice(Buffer* buf, U64 start, U64 end);
Slice bufAsSlice(Buffer* buf);
char* bufAsStr(Buffer* buf);

#endif