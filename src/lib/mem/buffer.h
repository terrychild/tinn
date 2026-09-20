#ifndef LIB_MEM_BUFFER_H
#define LIB_MEM_BUFFER_H

#include "lib/types.h"
#include "lib/mem/arena.h"

typedef struct {
    Arena* arena;
    U64 size;
    U64 length;
    U8* start;
} Buffer;

typedef struct {
    U64 length;
    U8* start;
} BufferSpace;

Buffer* bufNew(Arena* arena, U64 initial_size, U64 max_size);
void bufInit(Buffer* buf, Arena* arena, U64 initial_size, U64 max_size);
void bufReset(Buffer* buf);

void bufAppend(Buffer* buf, const U8* data, U64 n);
void bufAppendStr(Buffer* buf, const char* str);

BufferSpace bufReadyWrite(Buffer* buf, U64 min_size);
void bufConfirmWrite(Buffer* buf, U64 n);

char* bufAsStr(Buffer* buf);

void bufHexDump(Buffer* buf);

#endif