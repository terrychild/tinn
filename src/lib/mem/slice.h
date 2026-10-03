#ifndef LIB_MEM_SLICE_H
#define LIB_MEM_SLICE_H

#include "lib/types.h"

Slice sliceNew(Allocator* allocator, U64 length);
Slice sliceFromStr(const char* str);

I8 sliceCmp(const Slice a, const Slice b);
I8 sliceCmpStr(const Slice a, const char* b);
bool sliceIs(const Slice a, const Slice b);
bool sliceIsStr(const Slice a, const char* b);

Slice slice(const Slice source, U64 start, U64 end);

Slice sliceLeft(const Slice source, const Slice search);
Slice sliceLeftStr(const Slice source, const char* search);
Slice sliceLeftBack(const Slice source, const Slice search);
Slice sliceLeftBackStr(const Slice source, const char* search);
Slice sliceRight(const Slice source, const Slice search);
Slice sliceRightStr(const Slice source, const char* search);
Slice sliceRightBack(const Slice source, const Slice search);
Slice sliceRightBackStr(const Slice source, const char* search);

Slice sliceTrim(const Slice source);

Slice sliceToLowerCase(Slice source);
Slice sliceToUpperCase(Slice source);

typedef struct {
    Slice delim;
    Slice slice;
} Tokeniser;

Tokeniser sliceTokeniser(const Slice source, const Slice delim);
Tokeniser sliceTokeniserStr(const Slice source, const char* delim);
Slice nextToken(Tokeniser* tokeniser);

#endif