#include <string.h>

#include "lib/mem/reader.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"

Reader readerNew() {
    return (Reader) {
        .slice = sliceEmpty(),
    };
}
Reader readerFromSlice(const Slice slice) {
    return (Reader) {
        .slice = slice,
    };
}

Size readerRemaining(Reader* reader) {
    return reader->slice.length;
}
bool readerEnd(Reader* reader) {
    return reader->slice.length == 0;
}

void readSkip(Reader* reader, Size length) {
    reader->slice = slice(reader->slice, length, -1);
}
Slice readSlice(Reader* reader, Size length) {
    Slice rv = slice(reader->slice, 0, length);
    reader->slice = slice(reader->slice, length, -1);
    return rv;
}
Reader readReader(Reader* reader, Size length) {
    return readerFromSlice(readSlice(reader, length));
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