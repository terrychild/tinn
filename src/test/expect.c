#include <string.h>

#include "lib/types.h"
#include "lib/mem/slice.h"
//#include "lib/bigint.h"
#include "lib/bytes.h"
#include "lib/cli.h"

U64 expect_passed = 0;
U64 expect_failed = 0;

#define MAKE_EXPECT(T, F) \
void expect##T(const char* name, T value, T expected) { \
    if (value == expected) { \
        PRINT(CC_GREEN, "Passed"); \
        PRINT(CC_BRIGHT_WHITE, ": %s: ", name); \
        PRINT(CC_CYAN, F "\n", expected); \
        expect_passed++; \
    } else { \
        PRINT(CC_BRIGHT_RED, "Failed"); \
        PRINT(CC_BRIGHT_WHITE, ": %s, expected: ", name); \
        PRINT(CC_CYAN, F, expected); \
        PRINT(CC_BRIGHT_WHITE, " got: "); \
        PRINT(CC_MAGENTA, F "\n", value); \
        expect_failed++; \
    } \
}

MAKE_EXPECT(I8, "%d")
MAKE_EXPECT(I16, "%ld")
MAKE_EXPECT(I32, "%ld")
MAKE_EXPECT(I64, "%ld")
MAKE_EXPECT(U8, "%u")
MAKE_EXPECT(U16, "%lu")
MAKE_EXPECT(U32, "%lu")
MAKE_EXPECT(U64, "%lu")

void expectBool(const char* name, bool value, bool expected) {
    if (value == expected) {
        PRINT(CC_GREEN, "Passed");
        PRINT(CC_BRIGHT_WHITE, ": %s: ", name);
        PRINT(CC_CYAN, "%s\n", expected ? "true" : "false");
        expect_passed++;
    } else {
        PRINT(CC_BRIGHT_RED, "Failed");
        PRINT(CC_BRIGHT_WHITE, ": %s, expected: ", name);
        PRINT(CC_CYAN, "%s", expected ? "true" : "false");
        PRINT(CC_BRIGHT_WHITE, " got: ");
        PRINT(CC_MAGENTA, "%s\n", value ? "true" : "false");
        expect_failed++;
    }
}

void expectVoidPtr(const char* name, const void* value, const void* expected) {
    if (value == expected) {
        PRINT(CC_GREEN, "Passed");
        PRINT(CC_BRIGHT_WHITE, ": %s\n", name);
        expect_passed++;
    } else {
        PRINT(CC_BRIGHT_RED, "Failed");
        PRINT(CC_BRIGHT_WHITE, ": %s, expected: ", name);
        PRINT(CC_CYAN, "%lu", expected);
        PRINT(CC_BRIGHT_WHITE, " got: ");
        PRINT(CC_MAGENTA, "%lu\n", value);
        expect_failed++;
    }
}

void expectDiffVoidPtr(const char* name, const void* value, const void* expected) {
    if (value != expected) {
        PRINT(CC_GREEN, "Passed");
        PRINT(CC_BRIGHT_WHITE, ": %s\n", name);
        expect_passed++;
    } else {
        PRINT(CC_BRIGHT_RED, "Failed");
        PRINT(CC_BRIGHT_WHITE, ": %s, expected a difference, got: ", name);
        PRINT(CC_MAGENTA, "%lu\n", value);
        expect_failed++;
    }
}

void expectCharPtr(const char* name, const char* value, const char* expected) {
    if (strcmp(value, expected)==0) {
        PRINT(CC_GREEN, "Passed");
        PRINT(CC_BRIGHT_WHITE, ": %s: ", name);
        PRINT(CC_CYAN, "%s\n", expected);
        expect_passed++;
    } else {
        PRINT(CC_BRIGHT_RED, "Failed");
        PRINT(CC_BRIGHT_WHITE, ": %s, expected: ", name);
        PRINT(CC_CYAN, "%s", expected);
        PRINT(CC_BRIGHT_WHITE, " got: ");
        PRINT(CC_MAGENTA, "%s\n", value);
        expect_failed++;
    }
}

void expectSlice(const char* name, Slice value, Slice expected) {
    if (sliceCmp(value, expected)==0) {
        PRINT(CC_GREEN, "Passed");
        PRINT(CC_BRIGHT_WHITE, ": %s:\n", name);
        hexDump(expected);
        expect_passed++;
    } else {
        PRINT(CC_BRIGHT_RED, "Failed");
        PRINT(CC_BRIGHT_WHITE, ": %s, expected:\n", name);
        hexDump(expected);
        PRINT(CC_BRIGHT_WHITE, "got:\n");
        hexDump(value);
        expect_failed++;
    }
}

/*void expectBigInt(const char* name, BigInt* value, BigInt* expected) {
    if (biCmp(value, expected)==0) {
        PRINT(CC_GREEN, "Passed");
        PRINT(CC_BRIGHT_WHITE, ": %s:\n", name);
        hexDump(biToBESlice(expected));
        expect_passed++;
    } else {
        PRINT(CC_BRIGHT_RED, "Failed");
        PRINT(CC_BRIGHT_WHITE, ": %s, expected:\n", name);
        hexDump(biToBESlice(expected));
        PRINT(CC_BRIGHT_WHITE, "got:\n");
        hexDump(biToBESlice(value));
        expect_failed++;
    }
}*/

void expectNull(const char* name, void* value) {
    if (value==NULL) {
        PRINT(CC_GREEN, "Passed");
        PRINT(CC_BRIGHT_WHITE, ": %s: ", name);
        PRINT(CC_CYAN, "NULL\n");
        expect_passed++;
    } else {
        PRINT(CC_BRIGHT_RED, "Failed");
        PRINT(CC_BRIGHT_WHITE, ": %s, expected: ", name);
        PRINT(CC_CYAN, "NULL");
        PRINT(CC_BRIGHT_WHITE, " got something else\n");
        expect_failed++;
    }
}