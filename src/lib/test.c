#include "lib/types.h"
#include "lib/console.h"

bool expect_failed = false;

#define MAKE_EXPECT(T, F) \
void expect_##T(const char* name, T value, T expected) { \
    if (value == expected) { \
        PRINT(CC_GREEN, "Passed"); \
        PRINT(CC_BRIGHT_WHITE, ": %s\n", name); \
    } else { \
        PRINT(CC_BRIGHT_RED, "Failed") \
        PRINT(CC_BRIGHT_WHITE, ": %s, expected: ", name); \
        PRINT(CC_CYAN, F, expected); \
        PRINT(CC_BRIGHT_WHITE, " got: "); \
        PRINT(CC_MAGENTA, F "\n", value); \
        expect_failed = true; \
    } \
}

MAKE_EXPECT(i8, "%d")
MAKE_EXPECT(i64, "%ld")
MAKE_EXPECT(u8, "%u")
MAKE_EXPECT(u64, "%lu")