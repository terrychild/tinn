#include <string.h>

#include "lib/types.h"
#include "lib/console.h"

bool expect_failed = false;

#define MAKE_EXPECT(T, F) \
void expect##T(const char* name, T value, T expected) { \
    if (value == expected) { \
        PRINT(CC_GREEN, "Passed"); \
        PRINT(CC_BRIGHT_WHITE, ": %s\n", name); \
    } else { \
        PRINT(CC_BRIGHT_RED, "Failed"); \
        PRINT(CC_BRIGHT_WHITE, ": %s, expected: ", name); \
        PRINT(CC_CYAN, F, expected); \
        PRINT(CC_BRIGHT_WHITE, " got: "); \
        PRINT(CC_MAGENTA, F "\n", value); \
        expect_failed = true; \
    } \
}

MAKE_EXPECT(I8, "%d")
MAKE_EXPECT(I64, "%ld")
MAKE_EXPECT(U8, "%u")
MAKE_EXPECT(U64, "%lu")

void expectCharPtr(const char* name, const char* value, const char* expected) {
    if (strcmp(value, expected)==0) {
        PRINT(CC_GREEN, "Passed");
        PRINT(CC_BRIGHT_WHITE, ": %s\n", name);
    } else {
        PRINT(CC_BRIGHT_RED, "Failed");
        PRINT(CC_BRIGHT_WHITE, ": %s, expected: ", name);
        PRINT(CC_CYAN, "%s", expected);
        PRINT(CC_BRIGHT_WHITE, " got: ");
        PRINT(CC_MAGENTA, "%s\n", value);
        expect_failed = true;
    }
}