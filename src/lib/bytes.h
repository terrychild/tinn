#ifndef LIB_BYTES_H
#define LIB_BYTES_H

#include "lib/types.h"

U8 fromBE8(const U8 bytes[1]);
U16 fromBE16(const U8 bytes[2]);
U32 fromBE24(const U8 bytes[3]);
U32 fromBE32(const U8 bytes[4]);
U64 fromBE64(const U8 bytes[8]);

void toBE8(U8 dest[1], U8 n);
void toBE16(U8 dest[2], U16 n);
void toBE24(U8 dest[3], U32 n);
void toBE32(U8 dest[4], U32 n);
void toBE64(U8 dest[8], U64 n);

U8 readHexChar(char nibble);
char toHexChar(U8 nibble);
char* toHexStr(Memory* mem, Slice slice);
void hexDump(Slice slice);

#endif