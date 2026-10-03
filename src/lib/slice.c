#include <string.h>

#include "lib/slice.h"
#include "lib/mem/allocator.h"

Slice sliceNew(Allocator* allocator, U64 length) {
    return (Slice) {
        .length = length,
        .start = allocate(allocator, length)
    };
}
Slice sliceFromStr(const char* str) {
    return (Slice) {
        .length = strlen(str),
        .start = (const U8*)str
    };
}

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
bool sliceIs(const Slice a, const Slice b) {
    return sliceCmp(a, b) == 0;
}
bool sliceIsStr(const Slice a, const char* b) {
    return sliceCmpStr(a, b) == 0;
}

Slice slice(Slice slice, U64 start, U64 end) {
    if (start > slice.length) {
        start = slice.length;
    }
    if (end > slice.length) {
        end = slice.length;
    }
    return (Slice) {
        .length = end - start,
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

static Slice left(const Slice source, const Slice search, bool backwards) {
    bool found;
    U64 pos;
    if (backwards) {
        findLast(source, search, &found, &pos);
    } else {
        findFirst(source, search, &found, &pos);
    }
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
Slice sliceLeft(const Slice source, const Slice search) {
    return left(source, search, false);
}
Slice sliceLeftStr(const Slice source, const char* search) {
    return left(source, sliceFromStr(search), false);
}
Slice sliceLeftBack(const Slice source, const Slice search) {
    return left(source, search, true);
}
Slice sliceLeftBackStr(const Slice source, const char* search) {
    return left(source, sliceFromStr(search), true);
}

static Slice right(const Slice source, const Slice search, bool backwards) {
    bool found;
    U64 pos;
    if (backwards) {
        findLast(source, search, &found, &pos);
    } else {
        findFirst(source, search, &found, &pos);
    }
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
Slice sliceRight(const Slice source, const Slice search) {
    return right(source, search, false);
}
Slice sliceRightStr(const Slice source, const char* search) {
    return right(source, sliceFromStr(search), false);
}
Slice sliceRightBack(const Slice source, const Slice search) {
    return right(source, search, true);
}
Slice sliceRightBackStr(const Slice source, const char* search) {
    return right(source, sliceFromStr(search), true);
}

Slice sliceTrim(const Slice source) {
    U64 start = 0;
    while (start < source.length && (source.start[start] == ' ' || source.start[start] == '\t')) {
        start++;
    }
    U64 end = source.length;
    while (end > start && (source.start[end - 1] == ' ' || source.start[end - 1] == '\t')) {
        end--;
    }
    return (Slice) {
        .length = end - start,
        .start = source.start + start
    };
}

Slice sliceToLowerCase(Slice source) {
    for (U64 i=0; i<source.length; i++) {
        if (source.start[i] >= 'A' && source.start[i] <= 'Z') {
            ((U8*)source.start)[i] += 32;
        }
    }
    return source;
}
Slice sliceToUpperCase(Slice source) {
    for (U64 i=0; i<source.length; i++) {
        if (source.start[i] >= 'a' && source.start[i] <= 'z') {
            ((U8*)source.start)[i] -= 32;
        }
    }
    return source;
}

Tokeniser sliceTokeniser(const Slice source, const Slice delim) {
    return (Tokeniser) {
        .delim = delim,
        .slice = source
    };
}
Tokeniser sliceTokeniserStr(const Slice source, const char* delim) {
    return (Tokeniser) {
        .delim = sliceFromStr(delim),
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