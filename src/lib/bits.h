#ifndef LIB_BIT_H
#define LIB_BIT_H

#include "lib/types.h"

U8 rotl8(U8 value, uint count);
U32 rotl32(U32 value, uint count);
U64 rotl64(U64 value, uint count);

U8 rotr8(U8 value, uint count);
U32 rotr32(U32 value, uint count);
U64 rotr64(U64 value, uint count);

#endif