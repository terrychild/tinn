#include <string.h>

#include "lib/types.h"
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