#include <stdlib.h>
#include <string.h>
#include <stdbit.h>
#include <assert.h>

#include "lib/bigint.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/log.h"

static U64 left_bit_mask = 0x8000000000000000;

// === size and length ===

static void resize(BigInt* bi, U64 new_size, bool clear) {
    if (new_size > bi->size) {
        bi->size = new_size;
        bi->limbs = reallocate(bi->allocator, bi->limbs, sizeof(bi->limbs) * bi->size);
    } else if (clear && new_size < bi->size) {
        for (U64 limb = new_size; limb < bi->size; limb++) {
            bi->limbs[limb] = 0;
        }
    }
}
static void clear(BigInt* bi, U64 new_size) {
    resize(bi, new_size, false);
    for (U64 limb = 0; limb < bi->size; limb++) {
        bi->limbs[limb] = 0;
    }
}

static void updateLen(BigInt* bi) {
    for (U64 limb = bi->size; limb-- > 0; ) {
        if (bi->limbs[limb]) {
            bi->length = (limb * 64) + 64 - stdc_leading_zeros(bi->limbs[limb]);
            return;
        }
    }
    bi->length = 0;
}

static U64 bitsToSize(U64 bits) {
    U64 s = (bits + 63) / 64;
    return s ? s : 1;
}

static U64 normalSize(BigInt* bi) {
    return bitsToSize(bi->length);
}

static U64 maxNormalSize(BigInt* a, BigInt* b) {
    U64 as = normalSize(a);
    U64 bs = normalSize(b);
    return as >= bs ? as : bs;
}

// === make new big ints ===

BigInt* biNew(Allocator* allocator) {
    BigInt* bi = allocate(allocator, sizeof(BigInt));
    biInit(bi, allocator, 1);
    return bi;
}

void biInit(BigInt* bi, Allocator* allocator, U64 size) {
    assert(size > 0);

    bi->allocator = allocator;
    bi->size = size;
    bi->length = 0;
    bi->negative = false;
    bi->limbs = allocate(allocator, sizeof(bi->limbs) * bi->size);
    bi->mod = NULL;
}

static void copy(BigInt* dest, BigInt* source) {
    resize(dest, source->size, true);
    for (U64 limb = 0; limb < dest->size; limb++) {
        dest->limbs[limb] = limb < source->size ? source->limbs[limb] : 0;
    }
    dest->length = source->length;
    dest->negative = source->negative;
}
BigInt* biClone(Allocator* allocator, BigInt* source) {
    BigInt* dest = biNew(allocator);
    copy(dest, source);
    return dest;
}

// === input stuff ===

BigInt* biSetInt(BigInt* bi, I64 i) {
    resize(bi, 1, true);

    if (i>=0) {
        bi->negative = false;
        bi->limbs[0] = (U64)i;
    } else {
        bi->negative = true;
        bi->limbs[0] = (U64)(0-i);
    }
    updateLen(bi);

    return bi;
}
BigInt* biFromInt(Allocator* allocator, I64 i) {
    return biSetInt(biNew(allocator), i);
}

static U8 readHexChar(char nibble) {
	if (nibble < '0') {
		ERROR("Invalid hex char '%c'.\n", nibble);
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
		ERROR("Invalid hex char '%c'.\n", nibble);
		return 0;
	}
	return value;
}
BigInt* biSetHex(BigInt* bi, char* hex) {
    U64 nibbles = strlen(hex);
    U64 len = (nibbles+15)/16;

    resize(bi, len, true);

    U64 limb = 0;
    U64 bits = 0;
    U64 value = 0;
    for (U64 i = nibbles; i-- > 0; ) {
        value += ((U64)readHexChar(hex[i])) << bits;
        bits += 4;

        if (bits == 64) {
            bi->limbs[limb++] = value;
            bits = 0;
            value = 0;
        }
    }
    bi->limbs[limb] = value;
    updateLen(bi);
    bi->negative = false;

    return bi;
}
BigInt* biFromHex(Allocator* allocator, char* hex) {
    return biSetHex(biNew(allocator), hex);
}

void biSetMod(BigInt* bi, BigInt* mod) {
    bi->mod = mod;
}

// === output stuff ===

Slice biToBESlice(BigInt* bi) {
    if (bi->length == 0) {
        return sliceNew(allocate(bi->allocator, 1), 1);
    }

    U64 bytes_len = ((bi->length - 1) / 8) + 1;

    U8* bytes = allocate(bi->allocator, bytes_len);

    U64 i = 0;
    bool go = false;
    for (U64 limb = bi->size; limb-- > 0; ) {
        U64 mask = 0xff00000000000000;
        U64 bits = 56;
        while (mask) {
            if (!go && (bi->limbs[limb] & mask)) {
                go = true;
            }
            if (go) {
                bytes[i] = (bi->limbs[limb] & mask) >> bits;
                i++;
            }
            bits -= 8;
            mask >>= 8;
        }
    }

    return sliceNew(bytes, bytes_len);
}

// === comparison stuff ===

bool biIsZero(BigInt* bi) {
    return bi->length == 0;
}

int biCmpAbs(BigInt* a, BigInt* b) {
    if (a->length < b->length) {
        return -1;
    } else if (a->length > b->length) {
        return 1;
    } else {
        U64 limb = normalSize(a);
        while (limb-- > 0) {
            if (a->limbs[limb] < b->limbs[limb]) {
                return -1;
            } else if (a->limbs[limb] > b->limbs[limb]) {
                return 1;
            }
        }
        return 0;
    }
}

int biCmp(BigInt* a, BigInt* b) {
    if (a->negative == b->negative) {
        if (a->negative) {
            return biCmpAbs(b, a);
        }
        return biCmpAbs(a, b);
    } else if (a->negative) {
        return -1;
    } else {
        return 1;
    }
}

int biCmpInt(BigInt* a, I64 b) {
    if (a->negative == (b < 0)) {
        if (a->length > 64) {
            return a->negative ? -1 : 1;
        } else {
            U64 b_abs = b<0 ? (U64)0-b : (U64)b;
            if (a->limbs[0] < b_abs) {
                return a->negative ? 1 : -1;
            } else if (a->limbs[0] > b_abs) {
                return a->negative ? -1 : 1;
            } else {
                return 0;
            }
        }
    } else if (a->negative) {
        return -1;
    } else {
        return 1;
    }
}

bool biBit(BigInt* bi, U64 bit) {
    if (bit > bi->length) {
        return false;
    }
    U64 limb = bit / 64;
    U64 mask = (U64)1 << (bit - (limb * 64));
    return bi->limbs[limb] & mask;
}

// === bit shift stuff ===

BigInt* biShiftLeft(BigInt* dest, BigInt* source, U64 count) {
    if (biIsZero(source)) {
        biSetInt(dest, 0);
        return dest;
    }

    U64 move = count / 64;
    U64 shift = count % 64;
    U64 unshift = 64 - shift;
    U64 size = bitsToSize(source->length + count);

    resize(dest, size, true);

    U64 limb = size;
    if (shift) {
        while (limb-- > move + 1) {
            dest->limbs[limb] = (source->limbs[limb-move] << shift) | (source->limbs[limb-move-1] >> unshift);
        }
        dest->limbs[limb] = (source->limbs[limb-move] << shift);
    } else {
        while (limb > move) {
            limb--;
            dest->limbs[limb] = source->limbs[limb-move];
        }
    }
    if (move) {
        while (limb-- > 0) {
            dest->limbs[limb] = 0;
        }
    }

    dest->length = source->length + count;
    dest->negative = source->negative;

    if (dest->mod) {
        biMod(dest, dest, dest->mod);
    }

    return dest;
}

BigInt* biShiftRight(BigInt* dest, BigInt* source, U64 count) {
    if (source->length <= count) {
        biSetInt(dest, 0);
        return dest;
    }

    U64 move = count / 64;
    U64 shift = count % 64;
    U64 unshift = 64 - shift;
    U64 size = bitsToSize(source->length - count);

    resize(dest, size, false);

    U64 limb = 0;
    if (shift) {
        while (limb < size-1) {
            dest->limbs[limb] = (source->limbs[limb+move] >> shift) | (source->limbs[limb+move+1] << unshift);
            limb++;
        }
        if (limb+move+1 < source->size) {
            dest->limbs[limb] = (source->limbs[limb+move] >> shift) | (source->limbs[limb+move+1] << unshift);
        } else {
            dest->limbs[limb] = (source->limbs[limb+move] >> shift);
        }
        limb++;
    } else {
        while (limb < size) {
            dest->limbs[limb] = source->limbs[limb+move];
            limb++;
        }
    }
    while (limb < dest->size) {
        dest->limbs[limb] = 0;
        limb++;
    }

    dest->length = source->length - count;
    dest->negative = source->negative;

    if (dest->mod) {
        biMod(dest, dest, dest->mod);
    }

    return dest;
}

// === math stuff ===

static void add(BigInt* dest, BigInt* a, BigInt* b) {
    U64 size = maxNormalSize(a, b);
    resize(dest, size, true);

    uint64_t carry = 0;
    for (U64 i=0; i < size; i++) {
        U64 ai = 0;
        U64 bi = 0;
        if (i < a->size) {
            ai = a->limbs[i];
        }
        if (i < b->size) {
            bi = b->limbs[i];
        }
        dest->limbs[i] = ai + bi + carry;
        if (carry) {
            carry = dest->limbs[i] <= ai ? 1 : 0;
        } else {
            carry = dest->limbs[i] < ai ? 1 : 0;
        }
    }

    if (carry) {
        resize(dest, size + 1, false);
        dest->limbs[size] = 1;
    }
    updateLen(dest);
}

static void sub(BigInt* dest, BigInt* a, BigInt* b) {
    U64 size = maxNormalSize(a, b);
    resize(dest, size, true);

    U64 borrow = 0;
    for (U64 i=0; i < size; i++) {
        U64 ai = 0;
        U64 bi = 0;
        if (i < a->size) {
            ai = a->limbs[i];
        }
        if (i < b->size) {
            bi = b->limbs[i];
        }
        dest->limbs[i] = ai - bi - borrow;
        if (borrow) {
            borrow = bi >= ai ? 1 : 0;
        } else {
            borrow = bi > ai ? 1 : 0;
        }
    }

    if (borrow) {
        PANIC("Borrow left over!");
    }
    updateLen(dest);
}

BigInt* biAdd(BigInt* dest, BigInt* a, BigInt* b) {
    if (a->negative == b->negative) {
        add(dest, a, b);
        dest->negative = a->negative;
    } else {
        if (biCmpAbs(a, b) >= 0) {
            sub(dest, a, b);
            dest->negative = a->negative;
        } else {
            sub(dest, b, a);
            dest->negative = b->negative;
        }
    }
    if (dest->mod) {
        biMod(dest, dest, dest->mod);
    }
    if (biIsZero(dest)) {
        dest->negative = false;
    }
    return dest;
}

BigInt* biSub(BigInt* dest, BigInt* a, BigInt* b) {
    if (a->negative != b->negative) {
        add(dest, a, b);
        dest->negative = a->negative;
    } else {
        if (biCmpAbs(a, b) >= 0) {
            sub(dest, a, b);
            dest->negative = a->negative;
        } else {
            sub(dest, b, a);
            dest->negative = !b->negative;
        }
    }
    if (dest->mod) {
        biMod(dest, dest, dest->mod);
    }
    if (biIsZero(dest)) {
        dest->negative = false;
    }
    return dest;
}

BigInt* biMul(BigInt* dest, BigInt* a, BigInt* b) {
    if (biCmpAbs(a, b) < 0) {
        return biMul(dest, b, a);
    }
    if (biIsZero(b)) {
        clear(dest, 1);
        dest->length = 0;
        dest->negative = false;
    }

    U64 size = bitsToSize(a->length + b->length + 1);
    Allocator* temp_allocator = allocatorNew(size + b->size);
    BigInt* temp_b = biClone(temp_allocator, b);
    BigInt* temp_a = biClone(temp_allocator, a);

    clear(dest, size);
    dest->negative = a->negative != b->negative;

    U64 limb = 0;
    U64 mask = 1;
    U64 shift = 0;
    while (limb < temp_b->size) {
        if (temp_b->limbs[limb] & mask) {
            biShiftLeft(temp_a, temp_a, shift);
            shift = 0;
            add(dest, dest, temp_a);
        }
        shift++;
        mask <<= 1;
        if (!mask) {
            limb++;
            mask = 1;
        }
    }

    allocatorRelease(temp_allocator);

    if (dest->mod) {
        biMod(dest, dest, dest->mod);
    }

    return dest;
}

BigInt* biDiv(BigInt* dest, BigInt* a, BigInt* b, BigInt* r) {
    if (biIsZero(b)) {
        PANIC("Divide by zero!");
    }

    copy(r, a);
    Allocator* temp_allocator = allocatorNew(b->size * 4);
    BigInt* orig_b = biClone(temp_allocator, b);
    BigInt* temp_b = biClone(temp_allocator, b);
    BigInt* mask = biFromInt(temp_allocator, 1);

    biSetInt(dest, 0);
    dest->negative = a->negative != b->negative;

    U64 shift = 0;
    if (r->length > temp_b->length) {
        shift = r->length - b->length;
    }
    biShiftLeft(temp_b, temp_b, shift);
    if (biCmpAbs(temp_b, r) < 0) {
        biShiftLeft(temp_b, temp_b, 1);
        shift++;
    }
    biShiftLeft(mask, mask, shift);

    while (biCmpAbs(r, orig_b) >= 0) {
        if (biCmpAbs(r, temp_b) >= 0) {
            add(dest, dest, mask);
            sub(r, r, temp_b);
        }
        shift = 1;
        if (temp_b->length > r->length) {
            shift = temp_b->length - r->length;
        }
        biShiftRight(temp_b, temp_b, shift);
        biShiftRight(mask, mask, shift);
    }

    allocatorRelease(temp_allocator);

    if (dest->mod) {
        biMod(dest, dest, dest->mod);
    }

    return dest;
}

BigInt* biMod(BigInt* r, BigInt* a, BigInt* b) {
    if (biIsZero(b)) {
        PANIC("Divide by zero!");
    }

    copy(r, a);
    Allocator* temp_allocator = allocatorNew(b->size * 4);
    BigInt* orig_b = biClone(temp_allocator, b);
    BigInt* temp_b = biClone(temp_allocator, b);
    BigInt* mask = biFromInt(temp_allocator, 1);

    U64 shift = 0;
    if (r->length > temp_b->length) {
        shift = r->length - b->length;
    }
    biShiftLeft(temp_b, temp_b, shift);
    if (biCmpAbs(temp_b, r) < 0) {
        biShiftLeft(temp_b, temp_b, 1);
        shift++;
    }
    biShiftLeft(mask, mask, shift);

    while (biCmpAbs(r, orig_b) >= 0) {
        if (biCmpAbs(r, temp_b) >= 0) {
            sub(r, r, temp_b);
        }
        shift = 1;
        if (temp_b->length > r->length) {
            shift = temp_b->length - r->length;
        }
        biShiftRight(temp_b, temp_b, shift);
        biShiftRight(mask, mask, shift);
    }

    if (r->negative) {
        sub(r, b, r);
        r->negative = false;
    }

    allocatorRelease(temp_allocator);

    return r;
}

BigInt* biPow(BigInt* dest, BigInt* a, BigInt* b) {
    Allocator* temp_allocator = allocatorNew(a->size + b->size);
    BigInt* temp_a = biClone(temp_allocator, a);
    BigInt* temp_b = biClone(temp_allocator, b);

    copy(dest, a);

    bool go = false;
    U64 limb = temp_b->size;
    do {
        limb--;
        U64 mask = left_bit_mask;
        while (mask) {
            if (!go) {
                if (temp_b->limbs[limb] & mask) {
                    go = true;
                }
            } else {
                biMul(dest, dest, dest);
                if (temp_b->limbs[limb] & mask) {
                    biMul(dest, dest, temp_a);
                }
            }
            mask >>= 1;
        }

    } while (limb>0);

    allocatorRelease(temp_allocator);

    return dest;
}