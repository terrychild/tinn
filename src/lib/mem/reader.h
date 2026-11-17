#ifndef LIB_MEM_READER_H
#define LIB_MEM_READER_H

#include "lib/types.h"
#include "lib/macros.h"

struct Reader {
    Memory* mem;
    Size capacity;
    Size count;
    Slice* slices;
};

Reader* readerNew(Memory* mem);
Reader* readerFromSlice(Memory* mem, Slice slice);

void readerAppendSlice(Reader* reader, const Slice slice);
void readerAppendBytes(Reader* reader, const U8* start, Size length);
void readerAppendStr(Reader* reader, const char* str);
#define readerAppend(reader, ...) _Generic(FIRST_ARG(__VA_ARGS__), \
    const U8*: readerAppendBytes, \
    U8*: readerAppendBytes, \
    const Slice: readerAppendSlice, \
    Slice: readerAppendSlice, \
    const char*: readerAppendStr, \
    char*: readerAppendStr \
)(reader, __VA_ARGS__)

Size readerRemaining(Reader* reader);
bool readerEOF(Reader* reader);

void readSkip(Reader* reader, Size length);
Slice readSlice(Reader* reader, Size length);
Reader* readReader(Memory* mem, Reader* reader, Size length);
U8 readBE8(Reader* reader);
U16 readBE16(Reader* reader);
U32 readBE24(Reader* reader);
U32 readBE32(Reader* reader);

#endif
