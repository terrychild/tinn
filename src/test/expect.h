#ifndef TEST_EXPECT
#define TEST_EXPECT

#include "lib/types.h"

extern bool expect_failed;

void expectVoidPtr(const char* name, void* value, void* expected);
void expectI8(const char* name, U8 value, U8 expected);
void expectI64(const char* name, I64 value, I64 expected);
void expectU8(const char* name, U8 value, U8 expected);
void expectU64(const char* name, U64 value, U64 expected);
void expectCharPtr(const char* name, char* value, char* expected);
void expectNull(const char* name, void* value);

#define expect(name, value, expected) _Generic((value), \
        I8: expectI8, \
        U8: expectU8, \
        U64: expectU64, \
        I64: expectI64, \
        char*: expectCharPtr, \
        default: expectVoidPtr  \
    )(name, value, expected)

#endif