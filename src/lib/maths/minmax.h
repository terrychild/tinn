#ifndef LIB_MATHS_MINMAX_H
#define LIB_MATHS_MINMAX_H

#include "lib/types.h"

U64 min_U64(U64 a, U64 b);
I64 min_I64(I64 a, I64 b);

#define min(a, b) _Generic((a), \
    U64: min_U64, \
    default: min_I64 \
)(a, b)

#endif