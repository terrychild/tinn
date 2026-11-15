#include "lib/maths/minmax.h"

U64 min_U64(U64 a, U64 b) {
    return (a <= b) ? a : b;
}
I64 min_I64(I64 a, I64 b) {
    return (a <= b) ? a : b;
}