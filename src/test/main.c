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
        testMemory();
    }
    if (all || strcmp(argv[1], "array") == 0) {
        testArray();
    }
    if (all || strcmp(argv[1], "pool") == 0) {
        testPool();
    }
    if (all || strcmp(argv[1], "buffer") == 0) {
        testBuffer();
    }
    if (all || strcmp(argv[1], "maths") == 0) {
        testMaths();
    }
    if (all || strcmp(argv[1], "crypto") == 0) {
        testCrypto();
    }

    PRINT(CC_BLUE, "================\n Report\n================\n");
    PRINT(CC_MAGENTA, "Program memory: ");
    Memory* mem = programMemory();
    memoryDebug(mem);
    PRINT(CC_MAGENTA, "Scratch 1: ");
    ScratchMemory scratch = scratchMemory(NULL);
    memoryDebug(scratch.mem);
    PRINT(CC_MAGENTA, "Scratch 2: ");
    ScratchMemory scratch2 = scratchMemory(scratch.mem);
    memoryDebug(scratch2.mem);

    PRINT(CC_BRIGHT_WHITE, "Tests passed: ");
    if (expect_failed > 0) {
        PRINT(CC_BRIGHT_RED, "%lu / %lu\n", expect_passed, expect_passed + expect_failed);
        return EXIT_FAILURE;
    } else {
        PRINT(CC_BRIGHT_GREEN, "%lu / %lu\n", expect_passed, expect_passed);
        return EXIT_SUCCESS;
    }
}