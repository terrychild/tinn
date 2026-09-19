#include <string.h>

#include "lib/types.h"
#include "lib/cli.h"

bool expect_failed = false;

#define MAKE_EXPECT(T, F) \
void expect##T(const char* name, T value, T expected) { \
    if (value == expected) { \
        PRINTC(CC_GREEN, "Passed"); \
        PRINTC(CC_BRIGHT_WHITE, ": %s: ", name); \
        PRINTC(CC_CYAN, F "\n", expected); \
    } else { \
        PRINTC(CC_BRIGHT_RED, "Failed"); \
        PRINTC(CC_BRIGHT_WHITE, ": %s, expected: ", name); \
        PRINTC(CC_CYAN, F, expected); \
        PRINTC(CC_BRIGHT_WHITE, " got: "); \
        PRINTC(CC_MAGENTA, F "\n", value); \
        expect_failed = true; \
    } \
}

MAKE_EXPECT(I8, "%d")
MAKE_EXPECT(I64, "%ld")
MAKE_EXPECT(U8, "%u")
MAKE_EXPECT(U64, "%lu")

void expectVoidPtr(const char* name, const void* value, const void* expected) {
    if (value == expected) {
        PRINTC(CC_GREEN, "Passed");
        PRINTC(CC_BRIGHT_WHITE, ": %s", name);
    } else {
        PRINTC(CC_BRIGHT_RED, "Failed");
        PRINTC(CC_BRIGHT_WHITE, ": %s, expected: ", name);
        PRINTC(CC_CYAN, "%lu", expected);
        PRINTC(CC_BRIGHT_WHITE, " got: ");
        PRINTC(CC_MAGENTA, "%lu\n", value);
        expect_failed = true;
    }
}

void expectCharPtr(const char* name, const char* value, const char* expected) {
    if (strcmp(value, expected)==0) {
        PRINTC(CC_GREEN, "Passed");
        PRINTC(CC_BRIGHT_WHITE, ": %s:", name);
        PRINTC(CC_CYAN, "%s\n", expected);
    } else {
        PRINTC(CC_BRIGHT_RED, "Failed");
        PRINTC(CC_BRIGHT_WHITE, ": %s, expected: ", name);
        PRINTC(CC_CYAN, "%s", expected);
        PRINTC(CC_BRIGHT_WHITE, " got: ");
        PRINTC(CC_MAGENTA, "%s\n", value);
        expect_failed = true;
    }
}

void expectNull(const char* name, void* value) {
    if (value==NULL) {
        PRINTC(CC_GREEN, "Passed");
        PRINTC(CC_BRIGHT_WHITE, ": %s: ", name);
        PRINTC(CC_CYAN, "NULL\n");
    } else {
        PRINTC(CC_BRIGHT_RED, "Failed");
        PRINTC(CC_BRIGHT_WHITE, ": %s, expected: ", name);
        PRINTC(CC_CYAN, "NULL");
        PRINTC(CC_BRIGHT_WHITE, " got something else\n");
        expect_failed = true;
    }
}