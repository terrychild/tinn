#ifndef LIB_MEM_BUFFER_H
#define LIB_MEM_BUFFER_H

#include "lib/types.h"
#include "lib/macros.h"

struct Buffer {
    Memory* mem;
    U64 size;
    U64 length;
    U8* start;
};

typedef struct {
    U64 length;
    U8* start;
} BufferSpace;

Buffer* bufNew(Memory* mem, U64 size);
void bufInit(Buffer* buf, Memory* mem, U64 size);
void bufReset(Buffer* buf);

BufferSpace bufReadyWrite(Buffer* buf, U64 min_size);
void bufConfirmWrite(Buffer* buf, U64 n);

U64 bufReserve(Buffer* buf, U64 n);

void bufAppendBytes(Buffer* buf, const U8* data, U64 n);
void bufAppendSlice(Buffer* buf, const Slice slice);
void bufAppendMappedFile(Buffer* buf, const MappedFile* file);
void bufAppendStr(Buffer* buf, const char* format, ...);
void bufAppendBE8(Buffer* buf, U8 i);
void bufAppendBE16(Buffer* buf, U16 i);
void bufAppendBE24(Buffer* buf, U32 i);
void bufAppendBE32(Buffer* buf, U32 i);
void bufAppendBE64(Buffer* buf, U64 i);

#define bufAppend(buf, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const U8*: bufAppendBytes, \
    U8*: bufAppendBytes, \
    const Slice: bufAppendSlice, \
    Slice: bufAppendSlice, \
    const MappedFile*: bufAppendMappedFile, \
    MappedFile*: bufAppendMappedFile, \
    const char*: bufAppendStr, \
    char*: bufAppendStr \
)(buf, __VA_ARGS__)

void bufSetBE8(Buffer* buf, U64 offset, U8 i);
void bufSetBE16(Buffer* buf, U64 offset, U16 i);
void bufSetBE24(Buffer* buf, U64 offset, U32 i);
void bufSetBE32(Buffer* buf, U64 offset, U32 i);
void bufSetBE64(Buffer* buf, U64 offset, U64 i);

Slice bufAsSlice(Buffer* buf);
char* bufAsStr(Buffer* buf);

#endif