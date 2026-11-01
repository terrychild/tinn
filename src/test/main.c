#include <stdlib.h>
#include <string.h>

#include "test/tests.h"
#include "test/expect.h"
#include "lib/mem/memory.h"
#include "lib/mem/arena.h"
#include "lib/cli.h"

int main(int argc, char* argv[]) {
    bool all = true;
    if (argc >= 2) {
        all = false;
    }

    expect_passed = 0;
    expect_failed = 0;
    Memory* mem = memoryNew(0);

    if (all || strcmp(argv[1], "things") == 0) {
        testThings();
    }
    if (all || strcmp(argv[1], "types") == 0) {
        testTypes();
    }
    if (all || strcmp(argv[1], "arena") == 0) {
        testArena();
    }
    if (all || strcmp(argv[1], "memory") == 0) {
        testMemory(mem);
    }
    if (all || strcmp(argv[1], "array") == 0) {
        testArray(mem);
    }
    if (all || strcmp(argv[1], "pool") == 0) {
        testPool(mem);
    }
    if (all || strcmp(argv[1], "buffer") == 0) {
        testBuffer(mem);
    }
    if (all || strcmp(argv[1], "maths") == 0) {
        testMaths(mem);
    }
    if (all || strcmp(argv[1], "crypto") == 0) {
        testCrypto(mem);
    }

    PRINT(CC_BLUE, "================\n Report\n================\n");
    expect("Final memory check", mem->arena->allocated, sizeof(Arena) + sizeof(Memory));

    PRINT(CC_BRIGHT_WHITE, "Tests passed: ");
    if (expect_failed > 0) {
        PRINT(CC_BRIGHT_RED, "%lu / %lu\n", expect_passed, expect_passed + expect_failed);
        return EXIT_FAILURE;
    } else {
        PRINT(CC_BRIGHT_GREEN, "%lu / %lu\n", expect_passed, expect_passed);
        return EXIT_SUCCESS;
    }
}