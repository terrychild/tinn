#include <string.h>

#include "lib/slice.h"

I8 sliceCmp(const Slice a, const Slice b) {
    for (U64 i=0; i<a.length; i++) {
        if (i >= b.length) {
            return 1;
        }
        if (a.start[i] == b.start[i]) {
            continue;
        }
        return b.start[i] - a.start[i];
    }
    if (b.length > a.length) {
        return -1;
    }
    return 0;
}
I8 sliceCmpStr(const Slice a, const char* b) {
    return sliceCmp(a, (Slice){.length = strlen(b), .start = (const U8*)b});
}

Slice slice(Slice slice, U64 start, U64 length) {
    if (start > slice.length) {
        start = slice.length;
    }
    if (length == 0) {
        length = slice.length - start;
    } else {
        if (start + length > slice.length) {
            length = slice.length - start;
        }
    }
    return (Slice) {
        .length = length,
        .start = slice.start + start
    };
}

static void find(const Slice source, const Slice search, U64 start, U64 end, int direction, bool* found, U64* pos) {
    if (source.length == 0 || source.length < search.length) {
        *found = false;
        return;
    }
    if (search.length == 0) {
        *found = true;
        *pos = start;
        return;
    }
    U64 i = start;
    while(true) {
        if (sliceCmp((Slice){.length = search.length, .start = source.start + i}, search)==0) {
            *found = true;
            *pos = i;
            return;
        }
        if (i == end) {
            *found = false;
            return;
        }
        i += direction;
    }    
}
static void findFirst(const Slice source, const Slice search, bool* found, U64* pos) {
    find(source, search, 0, source.length - search.length, 1, found, pos);
}
static void findLast(const Slice source, const Slice search, bool* found, U64* pos) {
    find(source, search, source.length - search.length, 0, -1, found, pos);
}

Slice sliceLeft(const Slice source, const Slice search) {
    bool found;
    U64 pos;
    findFirst(source, search, &found, &pos);
    if (found) {
        return (Slice) {
            .length = pos,
            .start = source.start
        };
    } else {
        return (Slice) {
            .length = 0,
            .start = NULL
        };
    }
}
Slice sliceLeftStr(const Slice source, const char* search) {
    return sliceLeft(source, (Slice){.length = strlen(search), .start = (const U8*)search});
}
Slice sliceRight(const Slice source, const Slice search) {
    bool found;
    U64 pos;
    findLast(source, search, &found, &pos);
    if (found) {
        return (Slice) {
            .length = source.length - pos - search.length,
            .start = source.start + pos + search.length
        };
    } else {
        return (Slice) {
            .length = 0,
            .start = NULL
        };
    }
}
Slice sliceRightStr(const Slice source, const char* search) {
    return sliceRight(source, (Slice){.length = strlen(search), .start = (const U8*)search});
}

Tokeniser sliceTokeniser(const Slice source, const Slice delim) {
    return (Tokeniser) {
        .delim = delim,
        .slice = source
    };
}
Tokeniser sliceTokeniserStr(const Slice source, const char* delim) {
    return (Tokeniser) {
        .delim = {
            .length = strlen(delim),
            .start = (const U8*)delim
        },
        .slice = source
    };
}
Slice nextToken(Tokeniser* tokeniser) {
    bool found;
    U64 pos;
    findFirst(tokeniser->slice, tokeniser->delim, &found, &pos);
    if (found) {
        Slice rv = {
            .length = pos,
            .start = tokeniser->slice.start
        };
        tokeniser->slice = (Slice) {
            .length = tokeniser->slice.length - pos - tokeniser->delim.length,
            .start = tokeniser->slice.start + pos + tokeniser->delim.length
        };
        return rv;
    } else {
        Slice rv = tokeniser->slice;
        tokeniser->slice = (Slice) {
            .length = 0,
            .start = NULL
        };
        return rv;
    }
}