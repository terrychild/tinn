#include <stdio.h>

#include "lib/types.h"

void intToBytes32(U8 bytes[4], U32 n) {
    for (int i=0; i<32; i+=8) {
        bytes[3-(i/8)] = (n >> i) & 0xff;
    }
}
void intToBytes64(U8 bytes[8], U64 n) {
    for (int i=0; i<64; i+=8) {
        bytes[7-(i/8)] = (n >> i) & 0xff;
    }
}

U32 bytesToInt32(U8 bytes[4]) {
    U32 n = 0;
    for (int i=0; i<32; i+=8) {
        uint32_t temp_number = bytes[i/8];
        temp_number = temp_number << i;
        n |= (bytes[3-(i/8)] << i);
    }
    return n;
}
U64 bytesToInt64(U8 bytes[8]) {
    U64 n = 0;
    for (int i=0; i<64; i+=8) {
        n |= (bytes[7-(i/8)] << i);
    }
    return n;
}

void bytesToHex(char* hex, U8* bytes, U64 len) {
    for(U64 i=0; i<len; i++) {
        sprintf(hex+(i*2), "%02x", bytes[i]);
    }
}