#include <stdlib.h>
#include <stdio.h>

#include "lib.h"

int run_tests() {
    expect("u64 size", sizeof(u64), 8);
    expect("u8 range", (u8)(-1), 255);

    return EXIT_SUCCESS;
}