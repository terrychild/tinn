#ifndef LIB_BYTES_H
#define LIB_BYTES_H

#include "lib/types.h"

void intToBytes32(U8 bytes[4], U32 n);
void intToBytes64(U8 bytes[8], U64 n);

U32 bytesToInt32(uint8_t bytes[4]); 
U64 bytesToInt64(uint8_t bytes[8]);

void bytesToHex(char* hex, U8* bytes, U64 len);

#endif