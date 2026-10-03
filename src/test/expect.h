#ifndef TEST_EXPECT
#define TEST_EXPECT

#include "lib/types.h"

extern U64 expect_passed;
extern U64 expect_failed;

void expectVoidPtr(const char* name, void* value, void* expected);
void expectI8(const char* name, U8 value, U8 expected);
void expectI16(const char* name, I16 value, I16 expected);
void expectI32(const char* name, I32 value, I32 expected);
void expectI64(const char* name, I64 value, I64 expected);
void expectU8(const char* name, U8 value, U8 expected);
void expectU16(const char* name, U16 value, U16 expected);
void expectU32(const char* name, U32 value, U32 expected);
void expectU64(const char* name, U64 value, U64 expected);
void expectCharPtr(const char* name, char* value, char* expected);

#define expect(name, value, expected) _Generic((value), \
        I8: expectI8, \
        I16: expectI16, \
        I32: expectI32, \
        I64: expectI64, \
        U8: expectU8, \
        U16: expectU16, \
        U32: expectU32, \
        U64: expectU64, \
        char*: expectCharPtr, \
        default: expectVoidPtr  \
    )(name, value, expected)

void expectNull(const char* name, void* value);

#endif