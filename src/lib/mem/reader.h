#ifndef LIB_MEM_READER_H
#define LIB_MEM_READER_H

#include "lib/types.h"

typedef struct {
    Slice slice;
} Reader;

Reader readerNew();
Reader readerFromSlice(Slice slice);

Size readerRemaining(Reader* reader);
bool readerEnd(Reader* reader);

void readSkip(Reader* reader, Size length);
Slice readSlice(Reader* reader, Size length);
Reader readReader(Reader* reader, Size length);
U8 readBE8(Reader* reader);
U16 readBE16(Reader* reader);
U32 readBE24(Reader* reader);
U32 readBE32(Reader* reader);

#endif
