#ifndef LIB_BYTES_H
#define LIB_BYTES_H

#include "lib/types.h"

U16 fromBig16(const U8 bytes[2]);
U32 fromBig24(const U8 bytes[3]);
U32 fromBig32(const U8 bytes[4]);
U64 fromBig64(const U8 bytes[8]);

void hexDump(Slice slice);

#endif