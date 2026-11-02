#ifndef LIB_MACROS_H
#define LIB_MACROS_H

#include "lib/types.h"

#define STR_(a) #a
#define STR(a) STR_(a)
#define CONCAT_(a, b) a ## b
#define CONCAT(a, b) CONCAT_(a, b)

#define COUNT_ARGS_(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define COUNT_ARGS(...) COUNT_ARGS_(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define FIRST_ARG(first, ...) first

#define KiB(x) (U64) (x * 1024)
#define MiB(x) (U64) (x * 1024 * 1024)
#define GiB(x) (U64) (x * 1024 * 1024 * 1024)

#endif