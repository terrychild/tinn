#include <stdlib.h>

#include "test/expect.h"

#include "lib/types.h"
#include "lib/maths/bigint.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/bits.h"
#include "lib/cli.h"

void testMaths() {
    PRINT(CC_BLUE, "================\n Maths tests\n================\n");
    ScratchMemory scratch = scratchMemory(NULL);
    Memory* mem = scratch.mem;

    BigInt* bi = biFromInt(mem, 14);

    expect("bi 15", biAdd(bi, 1), biFromInt(mem, 15));
    expect("bi 30", biAdd(bi, bi), biFromInt(mem, 30));
    expect("bi 29", biSub(bi, 1), biFromInt(mem, 29));

    expect("cmp 29", biCmp(bi, 29), 0);
    expect("cmp 29", biCmp(bi, 30), -1);
    expect("cmp 29", biCmp(bi, 28), 1);

    expect("bi \"15\"", biFrom(mem, "15"), biFrom(mem, 15));
    expect("bi \"-15\"", biFrom(mem, "-15"), biFrom(mem, -15));
    expect("bi \"0xF\"", biFrom(mem, "0xF"), biFrom(mem, 15));
    expect("bi \"-0xF\"", biFrom(mem, "-0xF"), biFrom(mem, -15));

    expect("bi string", biToHexStr(mem, biSet(bi, "1024")), "0x400");

    U8 bytes[2] = {0x10, 0x20};
    U8 leBytes[2] = {0x20, 0x10};
    expect("Big Endian Bytes", biFromBE(mem, bytes, 2), biFrom(mem, 4128));
    expect("Little Endian Bytes", biSetLE(bi, bytes, 2), biFrom(mem, 8208));
    expect("To Big Endian Slice", biToBESlice(mem, bi), sliceNew(leBytes, 2));
    expect("To Little Endian Slice", biToLESlice(mem, bi), sliceNew(bytes, 2));

    biSet(bi, "0xFFFFFFFFFFFFFFFF");
    expect("bi FF* (size)", bi->size, 1);
    biAdd(bi, 1);
    expect("bi FF* + 1 (size)", bi->size, 2);
    expect("bi FF* + 1 (value)", bi, biFrom(mem, "0x10000000000000000"));
    expect("bi FF* + 1 (string)", biToHexStr(mem, bi), "0x10000000000000000");

    expect("shift right", biShiftRight(bi, 8), biFrom(mem, "0x100000000000000"));
    expect("shift left", biShiftLeft(bi, 8), biFrom(mem, "0x10000000000000000"));
    expect("shift left", biShiftLeft(bi, 1), biFrom(mem, "0x20000000000000000"));

    expect("8*8", biMul(bi, 8, 8), biFromInt(mem, 64));
    expect("square", biMul(bi, bi), biFromInt(mem, 4096));
    expect("square", biMul(bi, bi), biFromInt(mem, 16777216));
    expect("square", biMul(bi, bi), biFromInt(mem, 281474976710656));
    expect("square", biMul(bi, bi), biFrom(mem, "0x1000000000000000000000000"));

    BigInt* remainder = biNew(mem);
    expect("/3", biDiv(bi, bi, 3, remainder), biFrom(mem, "0x555555555555555555555555"));
    expect("remainder", remainder, biFromInt(mem, 1));
    expect("mod 1024", biMod(bi, 1024), biFromInt(mem, 341));

    expect("/4", biDiv(bi, 4), biFromInt(mem, 85));

    expect("2 pow 16", biPow(bi, 2, 16, NULL), biFromInt(mem, 65536));
    expect("2 pow 16 pow 16", biPow(bi, 16), biFrom(mem, "0x10000000000000000000000000000000000000000000000000000000000000000"));

    BigInt* bi_MAX = biFrom(mem, "0xFFFFFFFFFFFFFFFF");
    BigInt* bi_2MAX = biFrom(mem, "0x1FFFFFFFFFFFFFFFE");
    expect("big first", biMul(mem, bi_MAX, 2, NULL), bi_2MAX);
    expect("big second", biMul(mem, biSetInt(bi, 2), bi_MAX, NULL), bi_2MAX);

    expect("281474976710656/37", biDiv(bi, 281474976710656, 37, remainder), biFromInt(mem, 7607431802990));
    PRINT(CC_BLUE, "          %lu\n", bi->limbs[0]);
    expect("remainder", remainder, biFromInt(mem, 26));
    PRINT(CC_BLUE, "          %lu\n", remainder->limbs[0]);

    expect("0x3C & 0xF", biAnd(mem, 0x3C, 0xF), biFrom(mem, 0x0C));
    expect("0x3C | 0xF", biOr(mem, 0x3C, 0xF), biFrom(mem, 0x3F));
    expect("0x3C ^ 0xF", biXor(mem, 0x3C, 0xF), biFrom(mem, 0x33));
    expect("'a' ^ 32", biXor(mem, 'a', 32), biFrom(mem, 'A'));
    expect("'A' ^ 32", biXor(mem, 'A', 32), biFrom(mem, 'a'));

    U8 le_key[] = {0x9f, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf};
    BigInt* key = biFromLE(mem, le_key, 32);
    expect("little endian key", key, biFrom(mem, "0xafaeadacabaaa9a8a7a6a5a4a3a2a1a09f9e9d9c9b9a9998979695949392919f"));
    expect("clamped key", biAnd(key, biFrom(mem, "0x7ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff8")), biFrom(mem, "0x2faeadacabaaa9a8a7a6a5a4a3a2a1a09f9e9d9c9b9a99989796959493929198"));


    U32 rotate = 0x11;
    expect("rotate left 1", rotl32(rotate, 1), 0x22);
    expect("rotate right 1", rotr32(rotate, 1), 0x80000008);
    expect("rotate right 4", rotr32(rotate, 4), 0x10000001);
    expect("rotate right 8", rotr32(rotate, 8), 0x11000000);

    memoryDebug(mem);
    scratchRelease(scratch);
}