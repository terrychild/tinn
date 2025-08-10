#ifndef LIB_TEST
#define LIB_TEST

#include "lib/types.h"

void expect_i8(const char* name, i8 value, i8 expected);
void expect_i64(const char* name, i64 value, i64 expected);
void expect_u8(const char* name, u8 value, u8 expected);
void expect_u64(const char* name, u64 value, u64 expected);

#define expect(name, value, expected) _Generic((value), \
        i8: expect_u8, \
        u8: expect_u8, \
        u64: expect_u64, \
        default: expect_i64  \
    )(name, value, expected)

extern bool expect_failed;

#endif