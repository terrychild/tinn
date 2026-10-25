#include "lib/types.h"
#include "lib/cli.h"

U16 fromBE16(const U8 bytes[2]) {
    U16 n = 0;
    for (int i=0; i<2; i++) {
        n |= (U16)bytes[i] << 8*(1-i);
    }
    return n;
}

U32 fromBE24(const U8 bytes[3]) {
    U32 n = 0;
    for (int i=0; i<3; i++) {
        n |= (U32)bytes[i] << 8*(2-i);
    }
    return n;
}

U32 fromBE32(const U8 bytes[4]) {
    U32 n = 0;
    for (int i=0; i<4; i++) {
        n |= (U32)bytes[i] << 8*(3-i);
    }
    return n;
}

U64 fromBE64(const U8 bytes[8]) {
    U64 n = 0;
    for (int i=0; i<8; i++) {
        n |= (U64)bytes[i] << 8*(7-i);
    }
    return n;
}

void toBE16(U8 dest[2], U16 n) {
    for (int i=1; i>=0; i--) {
        dest[i] = n & 0xFF;
        n >>= 8;
    }
}
void toBE24(U8 dest[2], U32 n) {
    for (int i=2; i>=0; i--) {
        dest[i] = n & 0xFF;
        n >>= 8;
    }
}
void toBE32(U8 dest[2], U32 n) {
    for (int i=3; i>=0; i--) {
        dest[i] = n & 0xFF;
        n >>= 8;
    }
}
void toBE64(U8 dest[2], U64 n) {
    for (int i=7; i>=0; i--) {
        dest[i] = n & 0xFF;
        n >>= 8;
    }
}

void hexDump(Slice slice) {
    for (U64 i=0; i < slice.length; i+=16) {
        PRINT(CC_WHITE, "%08lX  ", i);
        for (U64 j=0; j<16; j++) {
            if (i+j < slice.length) {
                PRINT(CC_YELLOW, "%02X ", slice.start[i+j]);
            } else {
                PRINT(CC_BRIGHT_BLACK, ".. ");
            }
            if (j==7) {
                PRINT(CC_NULL, " ");
            }
        }
        PRINT(CC_NULL, " ");

        for (U64 j=0; j<16; j++) {
            if (i+j < slice.length && slice.start[i+j] > 32 && slice.start[i+j] < 127) {
                PRINT(CC_CYAN, "%c", slice.start[i+j]);
            } else {
                PRINT(CC_NULL, " ");
            }
        }
        PRINT(CC_NULL, "\n");
    }
}