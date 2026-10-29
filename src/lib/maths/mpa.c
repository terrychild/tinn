#include <stdbit.h>
#include <string.h>

#include "lib/maths/mpa.h"
#include "lib/mem/allocator.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/log.h"

// === input stuff ===

U64 mpaSetInt(U64 dest[], U64 limbs, U64 i) {
    dest[0] = i;
    for (U64 limb = 1; limb < limbs; limb++) {
        dest[limb] = 0;
    }

    return mpaBitLength(dest, limbs);
}

U64 mpaSetHex(U64 dest[], U64 limbs, char* hex) {
    U64 nibbles = strlen(hex);
    U64 len = (nibbles+15)/16;
    PANIC_IF(len > limbs, "Hex string exceeds number range");

    for (U64 limb = 0; limb < limbs; limb++) {
        dest[limb] = 0;
    }

    U64 limb = 0;
    U64 bits = 0;
    U64 value = 0;
    for (U64 i = nibbles; i-- > 0; ) {
        value += ((U64)readHexChar(hex[i])) << bits;
        bits += 4;

        if (bits == 64) {
            dest[limb++] = value;
            bits = 0;
            value = 0;
        }
    }
    dest[limb] = value;
    return mpaBitLength(dest, limbs);
}

// === output stuff ===

Slice mpaToBESlice(Allocator* allocator, U64 source[], U64 limbs) {
    U64 bit_len = mpaBitLength(source, limbs);
    if (bit_len == 0) {
        return sliceNew(allocate(allocator, 1), 1);
    }

    U64 bytes_len = ((bit_len - 1) / 8) + 1;

    U8* bytes = allocate(allocator, bytes_len);

    U64 i = 0;
    bool go = false;
    for (U64 limb = limbs; limb-- > 0; ) {
        U64 mask = 0xff00000000000000;
        U64 bits = 56;
        while (mask) {
            if (!go && (source[limb] & mask)) {
                go = true;
            }
            if (go) {
                bytes[i] = (source[limb] & mask) >> bits;
                i++;
            }
            bits -= 8;
            mask >>= 8;
        }
    }

    return sliceNew(bytes, bytes_len);
}

// === comparison stuff ===

U64 mpaBitLength(U64 source[], U64 limbs) {
    for (U64 limb = limbs; limb-- > 0; ) {
        if (source[limb]) {
            return (limb * 64) + 64 - stdc_leading_zeros(source[limb]);
        }
    }
    return 0;
}

bool mpaBit(U64 source[], U64 limbs, U64 bit) {
    U64 len = mpaBitLength(source, limbs);
    if (bit > len) {
        return false;
    }
    U64 limb = bit / 64;
    U64 mask = (U64)1 << (bit - (limb * 64));
    return source[limb] & mask;
}

bool mpaIsZero(U64 source[], U64 limbs) {
    return mpaBitLength(source, limbs) == 0;
}

int mpaCmp(U64 a[], U64 b[], U64 limbs) {
    U64 a_len = mpaBitLength(a, limbs);
    U64 b_len = mpaBitLength(b, limbs);
    if (a_len < b_len) {
        return -1;
    } else if (a_len > b_len) {
        return 1;
    } else {
        for (U64 limb = limbs; limb-- > 0; ) {
            if (a[limb] < b[limb]) {
                return -1;
            } else if (a[limb] > b[limb]) {
                return 1;
            }
        }
        return 0;
    }
}

int mpaCmpInt(U64 a[], U64 b, U64 limbs) {
    U64 a_len = mpaBitLength(a, limbs);
    if (a_len > 64) {
        return 1;
    } else {
        if (a[0] < b) {
            return -1;
        } else if (a[0] > b) {
            return 1;
        } else {
            return 0;
        }
    }
}

// === math stuff ===

U64 mpaAdd(U64 a[], U64 b[], U64 limbs) {
    U64 carry = 0;
    for (U64 i=0; i < limbs; i++) {
        U64 ai = a[i];
        U64 bi = b[i];
        a[i] = ai + bi + carry;
        if (carry) {
            carry = a[i] <= ai ? 1 : 0;
        } else {
            carry = a[i] < ai ? 1 : 0;
        }
    }
    return carry;
}

U64 mpaSub(U64 a[], U64 b[], U64 limbs) {
    U64 borrow = 0;
    for (U64 i=0; i < limbs; i++) {
        U64 ai = a[i];
        U64 bi = b[i];
        a[i] = ai - bi - borrow;
        if (borrow) {
            borrow = bi >= ai ? 1 : 0;
        } else {
            borrow = bi > ai ? 1 : 0;
        }
    }
    return borrow;
}