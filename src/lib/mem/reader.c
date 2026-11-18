#include <string.h>

#include "lib/mem/reader.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/log.h"

Reader* readerNew(Memory* mem, Size capacity) {
    Reader* reader = allocate(mem, sizeof(Reader));
    reader->slice.length = 0;
    reader->capacity = capacity;
    reader->count = 0;
    if (capacity > 0) {
        reader->slices = allocate(mem, capacity * sizeof(Slice));
    }
    return reader;
}

Reader sliceAsReader(Slice slice) {
    return (Reader) {
        .slice = {
            .length = slice.length,
            .start = slice.start,
        },
        .capacity = 0,
        .count = 0,
    };
}

Reader* readerFromSlice(Memory* mem, Slice slice) {
    Reader* reader = readerNew(mem, 0);
    readerAppendSlice(reader, slice);
    return reader;
}
Reader* readerFromArray(Memory* mem, Slice* slices, Size count) {
    Reader* reader = readerNew(mem, count - 1);
    for (Size i = 0; i<count; i++) {
        readerAppendSlice(reader, slices[i]);
    }
    return reader;
}

void readerAppendSlice(Reader* reader, const Slice slice) {
    if (reader->slice.length == 0) {
        reader->slice.length = slice.length;
        reader->slice.start = slice.start;
        return;
    }

    PANIC_IF(reader->count == reader->capacity, "Reader at capacity.");

    memcpy(reader->slices + reader->count, &slice, sizeof(Slice));
    reader->count++;
}
void readerAppendBytes(Reader* reader, const U8* start, Size length) {
    readerAppendSlice(reader, sliceNew(start, length));
}
void readerAppendStr(Reader* reader, const char* str) {
    readerAppendSlice(reader, sliceFromStr(str));
}
void readerAppendReader(Reader* dest, const Reader* source) {
    if (!readerEOF(source)) {
        readerAppendSlice(dest, source->slice);
        for (Size i=0; i<source->count; i++) {
            readerAppendSlice(dest, source->slices[i]);
        }
    }
}

Size readerRemaining(Reader* reader) {
    Size remaining = reader->slice.length;
    for (Size i=0; i<reader->count; i++) {
        remaining += reader->slices[i].length;
    }
    return remaining;
}
bool readerEOF(Reader* reader) {
    return reader->slice.length == 0;
}

void readSkip(Reader* reader, Size length) {
    readSlice(reader, length);
}
Slice readSlice(Reader* reader, Size length) {
    Slice rv = slice(reader->slice, 0, length);
    reader->slice = slice(reader->slice, length, -1);

    if (reader->slice.length == 0) {
        if (reader->count > 0) {
            reader->slice.length = reader->slices[0].length;
            reader->slice.start = reader->slices[0].start;
            reader->count--;
            if (reader->count > 0) {
                memmove(reader->slices, reader->slices + 1, reader->count * sizeof(Slice));
            }
        }
    }
    return rv;
}
Reader* readReader(Memory* mem, Reader* reader, Size length) {
    Reader* new_reader = readerNew(mem, reader->count);
    Size read = 0;
    while (read < length && !readerEOF(reader)) {
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