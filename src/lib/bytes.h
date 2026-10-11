#ifndef LIB_BYTES_H
#define LIB_BYTES_H

#include "lib/types.h"

U16 fromBig16(const U8 bytes[2]);
U32 fromBig24(const U8 bytes[3]);
U32 fromBig32(const U8 bytes[4]);
U64 fromBig64(const U8 bytes[8]);

void toBig16(U8 dest[2], U16 n);
void toBig24(U8 dest[2], U32 n);
void toBig32(U8 dest[2], U32 n);
void toBig64(U8 dest[2], U64 n);

void hexDump(Slice slice);

#endif