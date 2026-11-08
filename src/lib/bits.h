#ifndef LIB_BIT_H
#define LIB_BIT_H

#include "lib/types.h"

U8 rotl8(U8 value, U64 count);
U32 rotl32(U32 value, U64 count);
U64 rotl64(U64 value, U64 count);

U8 rotr8(U8 value, U64 count);
U32 rotr32(U32 value, U64 count);
U64 rotr64(U64 value, U64 count);

#endif