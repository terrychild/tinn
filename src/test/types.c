#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"

void testTypes() {
    PRINT(CC_BLUE, "================\n Type tests\n================\n");
    expect("U8 size", sizeof(U8), 1);
    expect("U64 size", sizeof(U64), 8);
    expect("U8 range", (U8)(-1), 255);
    expect("size_t size", sizeof(size_t), sizeof(U64));
}