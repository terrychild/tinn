#ifndef LIB_SLICE_H
#define LIB_SLICE_H

#include "lib/types.h"

I8 sliceCmp(const Slice a, const Slice b);
I8 sliceCmpStr(const Slice a, const char* b);

Slice slice(const Slice source, U64 start, U64 length);

Slice sliceLeft(const Slice source, const Slice search);
Slice sliceLeftStr(const Slice source, const char* search);
Slice sliceRight(const Slice source, const Slice search);
Slice sliceRightStr(const Slice source, const char* search);

typedef struct {
    Slice delim;
    Slice slice;
} Tokeniser;

Tokeniser sliceTokeniser(const Slice source, const Slice delim);
Tokeniser sliceTokeniserStr(const Slice source, const char* delim);
Slice nextToken(Tokeniser* tokeniser);

#endif