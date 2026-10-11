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
    expect("From big endian 16", fromBig16(data), 258);
    expect("From big endian 24", fromBig24(data), 66051);
    expect("From big endian 32", fromBig32(data), 16909060);
    expect("From big endian 64", fromBig64(data), 72623859790382856);

    U8 dest[8] = { 0, 0, 0, 0, 0, 0, 0, 0};
    toBig16(dest, 0x1234);
    expect("To big endian 16 [0]", dest[0], 0x12);
    expect("To big endian 16 [1]", dest[1], 0x34);

    toBig24(dest, 0x112233);
    expect("To big endian 24 [0]", dest[0], 0x11);
    expect("To big endian 24 [1]", dest[1], 0x22);
    expect("To big endian 24 [2]", dest[2], 0x33);

    toBig32(dest, 0x01020304);
    expect("To big endian 32 [0]", dest[0], 0x01);
    expect("To big endian 32 [1]", dest[1], 0x02);
    expect("To big endian 32 [2]", dest[2], 0x03);
    expect("To big endian 32 [3]", dest[3], 0x04);

    toBig64(dest, 0x1020304050607080);
    expect("To big endian 64 [0]", dest[0], 0x10);
    expect("To big endian 64 [1]", dest[1], 0x20);
    expect("To big endian 64 [2]", dest[2], 0x30);
    expect("To big endian 64 [3]", dest[3], 0x40);
    expect("To big endian 64 [4]", dest[4], 0x50);
    expect("To big endian 64 [5]", dest[5], 0x60);
    expect("To big endian 64 [6]", dest[6], 0x70);
    expect("To big endian 64 [7]", dest[7], 0x80);
}