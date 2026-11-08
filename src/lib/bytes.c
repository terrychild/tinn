#include "lib/mem/memory.h"
#include "lib/types.h"
#include "lib/log.h"
#include "lib/cli.h"

U8 fromBE8(const U8 bytes[1]) {
    return bytes[0];
}
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

void toBE8(U8 dest[1], U8 n) {
    dest[0] = n;
}
void toBE16(U8 dest[2], U16 n) {
    for (int i=1; i>=0; i--) {
        dest[i] = n & 0xFF;
        n >>= 8;
    }
}
void toBE24(U8 dest[3], U32 n) {
    for (int i=2; i>=0; i--) {
        dest[i] = n & 0xFF;
        n >>= 8;
    }
}
void toBE32(U8 dest[4], U32 n) {
    for (int i=3; i>=0; i--) {
        dest[i] = n & 0xFF;
        n >>= 8;
    }
}
void toBE64(U8 dest[8], U64 n) {
    for (int i=7; i>=0; i--) {
        dest[i] = n & 0xFF;
        n >>= 8;
    }
}

U8 readHexChar(char nibble) {
	if (nibble < '0') {
		ERROR("Invalid hex char '%c'.", nibble);
		return 0;
	}
	U8 value;
	if (nibble >= 'a') {
		value = nibble - 'a' + 10;
	} else if (nibble >= 'A') {
		value = nibble - 'A' + 10;
	} else {
		value = nibble - '0';
	}
	if (value > 15) {
		ERROR("Invalid hex char '%c'.", nibble);
		return 0;
	}
	return value;
}

char toHexChar(U8 nibble) {
	if (nibble < 10) {
        return nibble + '0';
	}
    if (nibble < 16) {
        return nibble + 'a' - 10;
    }
    ERROR("Invalid hex value '%d'.", nibble);
	return 'x';
}

char* toHexStr(Memory* mem, Slice slice) {
    char* str = allocate(mem, (slice.length * 2) + 3);
    str[0] = '0';
    str[1] = 'x';
    U64 byte = 2;
    for (U64 i = 0; i < slice.length; i++) {
        str[byte++] = toHexChar((slice.start[i] & 0xF0) >> 4);
        str[byte++] = toHexChar(slice.start[i] & 0x0F);
    }
    str[byte] = '\0';
    return str;
}

void hexDump(Slice slice) {
    for (U64 i=0; i < slice.length; i+=16) {
        PRINT(CC_WHITE, "%08lx  ", i);
        for (U64 j=0; j<16; j++) {
            if (i+j < slice.length) {
                PRINT(CC_YELLOW, "%02x ", slice.start[i+j]);
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