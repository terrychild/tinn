#include "test/expect.h"

#include "lib/types.h"
#include "lib/maths/mpa.h"
#include "lib/mem/allocator.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"

void testMaths(Allocator* allocator) {
    U64 key[4];
    expect("set int to 256 (bit length)", mpaSetInt(key, 4, 256), 9);
    U8 bytes_256[] = {1, 0};
    expect("set int to 256", mpaToBESlice(allocator, key, 4), sliceNew(bytes_256, 2));

    expect("set int to U64 max (bit length)", mpaSetHex(key, 4, "FFFFFFFFFFFFFFFF"), 64);
    U8 bytes_u64_max[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    expect("set int to U64 max", mpaToBESlice(allocator, key, 4), sliceNew(bytes_u64_max, 8));
    expect("cmp U64 max", mpaCmpInt(key, 0xFFFFFFFFFFFFFFFF, 4), 0);

    expect("set int to hex 42* (bit length)", mpaSetHex(key, 4, "4242424242424242424242424242424242424242424242424242424242424242"), 255);
    U8 bytes_42s[] = {
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42
    };
    expect("set int to hex 42*", mpaToBESlice(allocator, key, 4), sliceNew(bytes_42s, 32));

    U64 key_plus[4];
    mpaSetHex(key_plus, 4, "4242424242424242424242424242424242424242424242424242424242424243");
    U64 key_minus[4];
    mpaSetHex(key_minus, 4, "4242424242424242424242424242424242424242424242424242424242424241");
    expect("cmp plus", mpaCmp(key, key_plus, 4), -1);
    expect("cmp minus", mpaCmp(key, key_minus, 4), 1);

    U64 one[4];
    mpaSetInt(one, 4, 1);
    expect("add one (carry)", mpaAdd(key, one, 4), 0);
    U8 bytes_42s_plus[] = {
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x43
    };
    expect("add one (value)", mpaToBESlice(allocator, key, 4), sliceNew(bytes_42s_plus, 32));
    expect("add one (cmp)", mpaCmp(key, key_plus, 4), 0);

    U64 two[4];
    mpaSetInt(two, 4, 2);
    expect("minus two (borrow)", mpaSub(key, two, 4), 0);
    U8 bytes_42s_minus[] = {
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x41
    };
    expect("minus two (value)", mpaToBESlice(allocator, key, 4), sliceNew(bytes_42s_minus, 32));
    expect("minus two (cmp)", mpaCmp(key, key_minus, 4), 0);


    mpaSetHex(key, 4, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    expect("carry", mpaAdd(key, one, 4), 1);
    expect("is zero", mpaIsZero(key, 4), true);
    expect("borrow", mpaSub(key, one, 4), 1);

    U8 bytes_max[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    expect("max", mpaToBESlice(allocator, key, 4), sliceNew(bytes_max, 32));

    allocatorDebug(allocator);
    allocatorReset(allocator);
}