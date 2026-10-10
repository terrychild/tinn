#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/bytes.h"
#include "lib/cli.h"

void testTypes() {
    PRINT(CC_BLUE, "================\n Type tests\n================\n");
    expect("U8 size", sizeof(U8), 1);
    expect("U64 size", sizeof(U64), 8);
    expect("U8 range", (U8)(-1), 255);
    expect("size_t size", sizeof(size_t), sizeof(U64));

    U8 data[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    expect("Big endian 16", fromBig16(data), 258);
    expect("Big endian 24", fromBig24(data), 66051);
    expect("Big endian 32", fromBig32(data), 16909060);
    expect("Big endian 64", fromBig64(data), 72623859790382856);
}