#ifndef LIB_TEST
#define LIB_TEST

#include "lib/types.h"

void expectI8(const char* name, U8 value, U8 expected);
void expectI64(const char* name, I64 value, I64 expected);
void expectU8(const char* name, U8 value, U8 expected);
void expectU64(const char* name, U64 value, U64 expected);

#define expect(name, value, expected) _Generic((value), \
        I8: expectI8, \
        U8: expectU8, \
        U64: expectU64, \
        default: expectI64  \
    )(name, value, expected)

extern bool expect_failed;

#endif