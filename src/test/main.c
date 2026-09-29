#include <stdlib.h>
#include <string.h>

#include "test/tests.h"
#include "test/expect.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"
#include "lib/cli.h"

int main(int argc, char* argv[]) {
    bool all = true;
    if (argc >= 2) {
        all = false;
    }

    expect_passed = 0;
    expect_failed = 0;
    Allocator* allocator = allocatorNew();

    if (all || strcmp(argv[1], "types") == 0) {
        testTypes();
    }
    if (all || strcmp(argv[1], "arena") == 0) {
        testArena();
    }
    if (all || strcmp(argv[1], "allocator") == 0) {
        testAllocator(allocator);
    }
    if (all || strcmp(argv[1], "array") == 0) {
        testArray(allocator);
    }
    if (all || strcmp(argv[1], "pool") == 0) {
        testPool(allocator);
    }
    if (all || strcmp(argv[1], "buffer") == 0) {
        testBuffer(allocator);
    }

    PRINT(CC_BLUE, "================\n Report\n================\n");
    expect("Final allocator check", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator));

    PRINT(CC_BRIGHT_WHITE, "Tests passed: ");
    if (expect_failed > 0) {
        PRINT(CC_BRIGHT_RED, "%lu / %lu\n", expect_passed, expect_passed + expect_failed);
        return EXIT_FAILURE;
    } else {
        PRINT(CC_BRIGHT_GREEN, "%lu / %lu\n", expect_passed, expect_passed);
        return EXIT_SUCCESS;
    }
}