#include <string.h>

#include "lib/mem/reader.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"

Reader* readerNew(Memory* mem) {
    Reader* reader = allocate(mem, sizeof(Reader));
    reader->mem = mem;
    reader->capacity = 4;
    reader->count = 0;
    reader->slices = allocate(reader->mem, reader->capacity * sizeof(Slice));
    return reader;
}

Reader* readerFromSlice(Memory* mem, Slice slice) {
    Reader* reader = readerNew(mem);
    readerAppendSlice(reader, slice);
    return reader;
}

static void* add(Reader* reader) {
    if (reader->count == reader->capacity) {
        reader->capacity *= 2;
        Size size = reader->capacity * sizeof(Slice);
        reader->slices = reallocate(reader->mem, reader->slices, size);
    }

    void* address = reader->slices + (reader->count * sizeof(Slice));
    reader->count++;
    return address;
}

void readerAppendSlice(Reader* reader, const Slice slice) {
    memcpy(add(reader), &slice, sizeof(Slice));
}
void readerAppendBytes(Reader* reader, const U8* start, Size length) {
    readerAppendSlice(reader, sliceNew(start, length));
}
void readerAppendStr(Reader* reader, const char* str) {
    readerAppendSlice(reader, sliceFromStr(str));
}

Size readerRemaining(Reader* reader) {
    Size remaining = 0;
    for (Size i=0; i<reader->count; i++) {
        remaining += reader->slices[i].length;
    }
    return remaining;
}
bool readerEOF(Reader* reader) {
    return reader->count == 0;
}

void readSkip(Reader* reader, Size length) {
    readSlice(reader, length);
}
Slice readSlice(Reader* reader, Size length) {
    if (reader->count == 0) {
        return sliceEmpty();
    }
    Slice rv = slice(reader->slices[0], 0, length);
    reader->slices[0] = slice(reader->slices[0], length, -1);
    if (reader->slices[0].length == 0) {
        reader->count--;
        if (reader->count > 0) {
            memmove(reader->slices, reader->slices + 1, reader->count * sizeof(Slice));
        }
    }
    return rv;
}
Reader* readReader(Memory* mem, Reader* reader, Size length) {
    Reader* new_reader = readerNew(mem);
    Size read = 0;
    for (Size i=0; i<reader->count && read < length; i++) {
        Slice slice = readSlice(reader, length - read);
        readerAppendSlice(new_reader, slice);
        read += slice.length;
    }
    return new_reader;
}
U8 readBE8(Reader* reader) {
    Slice slice = readSlice(reader, 1);
    if (slice.length != 1) {
        return (U8)0;
    }
    return slice.start[0];
}
U16 readBE16(Reader* reader) {
    Slice slice = readSlice(reader, 2);
    if (slice.length != 2) {
        return (U16)0;
    }
    return fromBE16(slice.start);
}
U32 readBE24(Reader* reader) {
    Slice slice = readSlice(reader, 3);
    if (slice.length != 3) {
        return (U32)0;
    }
    return fromBE24(slice.start);
}
U32 readBE32(Reader* reader) {
    Slice slice = readSlice(reader, 4);
    if (slice.length != 4) {
        return (U32)0;
    }
    return fromBE32(slice.start);
}