#include <limits.h>

#include "lib/types.h"

U8 rotl8(U8 value, U8 count) {
    const U64 mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

U32 rotl32(U32 value, U8 count) {
    const U64 mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}