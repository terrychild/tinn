#include <stdlib.h>

#include "test/expect.h"
#include "lib/lib.h"

int runTests() {
    expect_failed = false;

    PRINTC(CC_BLUE, "================\n Type tests\n================\n");

    expect("U8 size", sizeof(U8), 1);
    expect("U64 size", sizeof(U64), 8);
    expect("U8 range", (U8)(-1), 255);
    expect("size_t size", sizeof(size_t), sizeof(U64));

    PRINTC(CC_BLUE, "================\n Report\n================\n");
    if (expect_failed) {
        PRINTC(CC_BRIGHT_RED, "Some tests failed!\n");
        return EXIT_FAILURE;
    } else {
        PRINTC(CC_GREEN, "All tests passed!\n");
        return EXIT_SUCCESS;
    }
}