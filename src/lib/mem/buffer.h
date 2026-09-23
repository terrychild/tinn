#ifndef LIB_MEM_BUFFER_H
#define LIB_MEM_BUFFER_H

#include "lib/types.h"

struct Buffer {
    Arena* arena;
    U64 size;
    U64 length;
    U8* start;
};

typedef struct {
    U64 length;
    U8* start;
} BufferSpace;

typedef struct Slice {
    U64 length;
    U8* start;
} Slice;

Buffer* bufNew(Allocator* allocator, U64 initial_size, U64 max_size);
void bufInit(Buffer* buf, Arena* arena, U64 initial_size);
void bufReset(Buffer* buf);

void bufAppend(Buffer* buf, const U8* data, U64 n);
void bufAppendStr(Buffer* buf, const char* str);

BufferSpace bufReadyWrite(Buffer* buf, U64 min_size);
void bufConfirmWrite(Buffer* buf, U64 n);

Slice bufAsSlice(Buffer* buf);
char* bufAsStr(Buffer* buf);

void bufHexDump(Buffer* buf);

#endif