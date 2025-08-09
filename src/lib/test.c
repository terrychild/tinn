#include "test.h"

#define MAKE_EXPECT(T, F) \
bool expect_##T(const char* name, T value, T expected) { \
    if (value == expected) { \
        PRINT(CC_GREEN, "Passed"); \
        PRINT(CC_WHITE, ": %s\n", name); \
        return true; \
    } \
    PRINT(CC_BOLD_RED, "Failed") \
    PRINT(CC_WHITE, ": %s, expected: ", name); \
    PRINT(CC_CYAN, F, expected); \
    PRINT(CC_WHITE, " got: "); \
    PRINT(CC_MAGENTA, F "\n", value); \
    return false; \
}

MAKE_EXPECT(i8, "%d")
MAKE_EXPECT(i64, "%ld")
MAKE_EXPECT(u8, "%u")
MAKE_EXPECT(u64, "%lu")