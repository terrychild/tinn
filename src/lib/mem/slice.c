#include <string.h>

#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/mem/buffer.h"
#include "lib/bytes.h"

Slice sliceNew(const U8* start, Size length) {
    return (Slice) {
        .length = length,
        .start = start
    };
}
Slice sliceFromStr(const char* str) {
    return (Slice) {
        .length = strlen(str),
        .start = (const U8*)str
    };
}
Slice sliceEmpty() {
    return (Slice) {
        .length = 0,
        .start = NULL
    };
}


Slice sliceFromHex(Memory* mem, const char* hex) {
    Size nibbles = strlen(hex);
    Size count = (nibbles + 1) / 2;
    U8* bytes = allocate(mem, count);
    readHexStr(bytes, nibbles, hex);
    return sliceNew(bytes, count);
}
char* sliceToHex(Memory* mem, Slice slice) {
    char* str = allocate(mem, (slice.length * 2) + 1);
    Size byte = 0;
    for (Size i = 0; i < slice.length; i++) {
        str[byte++] = toHexChar((slice.start[i] & 0xF0) >> 4);
        str[byte++] = toHexChar(slice.start[i] & 0x0F);
    }
    str[byte] = '\0';
    return str;
}

I8 sliceCmp(const Slice a, const Slice b) {
    for (Size i=0; i<a.length; i++) {
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

Slice sliceSlice(Slice slice, Size start, Size end) {
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
Slice sliceBuffer(Buffer* buf, Size start, Size end) {
    if (start > buf->length) {
        start = buf->length;
    }
    if (end > buf->length) {
        end = buf->length;
    }
    return (Slice) {
        .length = end - start,
        .start = buf->start + start
    };
}

static void find(const Slice source, const Slice search, Size start, Size end, int direction, bool* found, Size* pos) {
    if (source.length == 0 || source.length < search.length) {
        *found = false;
        return;
    }
    if (search.length == 0) {
        *found = true;
        *pos = start;
        return;
    }
    Size i = start;
    while(true) {
        if (sliceCmp(slice(source, i, i + search.length), search)==0) {
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
static void findFirst(const Slice source, const Slice search, bool* found, Size* pos) {
    find(source, search, 0, source.length - search.length, 1, found, pos);
}
static void findLast(const Slice source, const Slice search, bool* found, Size* pos) {
    find(source, search, source.length - search.length, 0, -1, found, pos);
}

static Slice left(const Slice source, const Slice search, bool backwards) {
    bool found;
    Size pos;
    if (backwards) {
        findLast(source, search, &found, &pos);
    } else {
        findFirst(source, search, &found, &pos);
    }
    if (found) {
        return slice(source, 0, pos);
    } else {
        return sliceEmpty();
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
    Size pos;
    if (backwards) {
        findLast(source, search, &found, &pos);
    } else {
        findFirst(source, search, &found, &pos);
    }
    if (found) {
        return slice(source, pos + 1, -1);
    } else {
        return sliceEmpty();
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
    Size start = 0;
    while (start < source.length && (source.start[start] == ' ' || source.start[start] == '\t')) {
        start++;
    }
    Size end = source.length;
    while (end > start && (source.start[end - 1] == ' ' || source.start[end - 1] == '\t')) {
        end--;
    }
    return slice(source, start, end);
}

Slice sliceToLowerCase(Memory* mem, const Slice source) {
    U8* target = allocate(mem, source.length);
    for (Size i=0; i<source.length; i++) {
        if (source.start[i] >= 'A' && source.start[i] <= 'Z') {
            target[i] = source.start[i] + 32;
        } else {
            target[i] = source.start[i];
        }
    }
    return sliceNew(target, source.length);
}
Slice sliceToUpperCase(Memory* mem, const Slice source) {
    U8* target = allocate(mem, source.length);
    for (Size i=0; i<source.length; i++) {
        if (source.start[i] >= 'a' && source.start[i] <= 'z') {
            target[i] = source.start[i] - 32;
        } else {
            target[i] = source.start[i];
        }
    }
    return sliceNew(target, source.length);
}

Tokeniser sliceTokeniser(const Slice source, const Slice delim, bool greedy) {
    return (Tokeniser) {
        .slice = source,
        .delim = delim,
        .greedy = greedy
    };
}
Tokeniser sliceTokeniserStr(const Slice source, const char* delim, bool greedy) {
    return (Tokeniser) {
        .slice = source,
        .delim = sliceFromStr(delim),
        .greedy = greedy
    };
}
Slice nextToken(Tokeniser* tokeniser) {
    bool found;
    Size pos;
    findFirst(tokeniser->slice, tokeniser->delim, &found, &pos);
    if (found) {
        Size next_pos = pos + tokeniser->delim.length;
        if (tokeniser->greedy) {
            while (sliceIs(slice(tokeniser->slice, next_pos, next_pos + tokeniser->delim.length), tokeniser->delim)) {
                next_pos += tokeniser->delim.length;
            }
        }
        Slice rv = slice(tokeniser->slice, 0, pos);
        tokeniser->slice = slice(tokeniser->slice, next_pos, -1);
        return rv;
    } else {
        Slice rv = tokeniser->slice;
        tokeniser->slice = sliceEmpty();
        return rv;
    }
}