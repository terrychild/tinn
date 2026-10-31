#include <string.h>

#include "lib/maths/bigint.h"
#include "lib/maths/mpa.h"
#include "lib/mem/allocator.h"
#include "lib/log.h"

static U64 left_bit_mask = 0x8000000000000000;

// === size and length ===

static void resize(BigInt* bi, U64 new_size, bool clear_extra) {
    if (new_size > bi->size) {
        bi->size = new_size;
        bi->limbs = reallocate(bi->allocator, bi->limbs, sizeof(bi->limbs) * bi->size);
    } else if (clear_extra && new_size < bi->size) {
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
    bi->length = mpaBitLength(bi->limbs, bi->size);
}

static U64 bitsToSize(U64 bits) {
    U64 s = (bits + 63) / 64;
    return s ? s : 1;
}

static U64 normalSize(const BigInt* bi) {
    return bitsToSize(bi->length);
}

static U64 maxNormalSize(const BigInt* a, const BigInt* b) {
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
    PANIC_IF(size == 0, "BigInt size cannot be 0.");

    bi->allocator = allocator;
    bi->size = size;
    bi->length = 0;
    bi->negative = false;
    bi->limbs = allocate(allocator, sizeof(bi->limbs) * bi->size);
    bi->mod = NULL;
}

static void biInitInt(BigInt* bi, U64 limbs[], I64 i) {
    limbs[0] = i<0 ? (U64)0-i : (U64)i;

    bi->allocator = NULL;
    bi->size = 1;
    bi->length = mpaBitLength(limbs, 1);
    bi->negative = i<0;
    bi->limbs = limbs;
    bi->mod = NULL;
}

BigInt* biCopy(BigInt* dest, const BigInt* source) {
    resize(dest, source->size, true);
    for (U64 limb = 0; limb < dest->size; limb++) {
        dest->limbs[limb] = limb < source->size ? source->limbs[limb] : 0;
    }
    dest->length = source->length;
    dest->negative = source->negative;
    return dest;
}
BigInt* biClone(Allocator* allocator, const BigInt* source) {
    return biCopy(biNew(allocator), source);
}

// === input stuff ===

BigInt* biSetInt(BigInt* bi, I64 i) {
    if (i>=0) {
        bi->negative = false;
        bi->length = mpaSetInt(bi->limbs, bi->size, (U64)i);
    } else {
        bi->negative = true;
        bi->length = mpaSetInt(bi->limbs, bi->size, (U64)(0-i));
    }
    return bi;
}
BigInt* biFromInt(Allocator* allocator, I64 i) {
    return biSetInt(biNew(allocator), i);
}

BigInt* biSetHex(BigInt* bi, const char* hex) {
    U64 nibbles = strlen(hex);
    U64 len = (nibbles+15)/16;

    resize(bi, len, true);
    bi->length = mpaSetHex(bi->limbs, len, hex);
    bi->negative = false;

    return bi;
}
BigInt* biFromHex(Allocator* allocator, const char* hex) {
    return biSetHex(biNew(allocator), hex);
}

void biSetMod(BigInt* bi, BigInt* mod) {
    bi->mod = mod;
}

// === output stuff ===

Slice biToBESlice(Allocator* allocator, const BigInt* bi) {
    return mpaToBESlice(allocator, bi->limbs, bi->size);
}

// === comparison stuff ===

bool biBit(const BigInt* bi, U64 bit) {
    if (bit > bi->length) {
        return false;
    }
    U64 limb = bit / 64;
    U64 mask = (U64)1 << (bit - (limb * 64));
    return bi->limbs[limb] & mask;
}

bool biIsZero(const BigInt* bi) {
    return bi->length == 0;
}

int biCmpAbs(const BigInt* a, const BigInt* b) {
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

int biCmpBi(const BigInt* a, const BigInt* b) {
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

int biCmpInt(const BigInt* a, I64 b) {
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

// === bit shift stuff ===

BigInt* biShiftLeft(BigInt* bi, U64 count) {
    if (biIsZero(bi)) {
        biSetInt(bi, 0);
        return bi;
    }

    U64 move = count / 64;
    U64 shift = count % 64;
    U64 unshift = 64 - shift;
    U64 size = bitsToSize(bi->length + count);

    resize(bi, size, true);

    U64 limb = size;
    if (shift) {
        while (limb-- > move + 1) {
            bi->limbs[limb] = (bi->limbs[limb-move] << shift) | (bi->limbs[limb-move-1] >> unshift);
        }
        bi->limbs[limb] = (bi->limbs[limb-move] << shift);
    } else {
        while (limb > move) {
            limb--;
            bi->limbs[limb] = bi->limbs[limb-move];
        }
    }
    if (move) {
        while (limb-- > 0) {
            bi->limbs[limb] = 0;
        }
    }

    bi->length = bi->length + count;

    if (bi->mod) {
        biMod(bi, bi->mod);
    }

    return bi;
}

BigInt* biShiftRight(BigInt* bi, U64 count) {
    if (bi->length <= count) {
        biSetInt(bi, 0);
        return bi;
    }

    U64 move = count / 64;
    U64 shift = count % 64;
    U64 unshift = 64 - shift;
    U64 size = bitsToSize(bi->length - count);

    resize(bi, size, false);

    U64 limb = 0;
    if (shift) {
        while (limb < size-1) {
            bi->limbs[limb] = (bi->limbs[limb+move] >> shift) | (bi->limbs[limb+move+1] << unshift);
            limb++;
        }
        if (limb+move+1 < bi->size) {
            bi->limbs[limb] = (bi->limbs[limb+move] >> shift) | (bi->limbs[limb+move+1] << unshift);
        } else {
            bi->limbs[limb] = (bi->limbs[limb+move] >> shift);
        }
        limb++;
    } else {
        while (limb < size) {
            bi->limbs[limb] = bi->limbs[limb+move];
            limb++;
        }
    }
    while (limb < bi->size) {
        bi->limbs[limb] = 0;
        limb++;
    }

    bi->length = bi->length - count;

    return bi;
}

// === math stuff ===

static void add(BigInt* a, const BigInt* b) {
    U64 size = maxNormalSize(a, b);
    resize(a, size, true);

    U64 carry = 0;
    for (U64 i=0; i < size; i++) {
        U64 ai = 0;
        U64 bi = 0;
        if (i < a->size) {
            ai = a->limbs[i];
        }
        if (i < b->size) {
            bi = b->limbs[i];
        }
        a->limbs[i] = ai + bi + carry;
        if (carry) {
            carry = a->limbs[i] <= ai ? 1 : 0;
        } else {
            carry = a->limbs[i] < ai ? 1 : 0;
        }
    }

    if (carry) {
        resize(a, size + 1, false);
        a->limbs[size] = 1;
    }
    updateLen(a);
}

static void sub(BigInt* dest, const BigInt* a, const BigInt* b) {
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

    PANIC_IF(borrow>0, "Borrow left over!");
    updateLen(dest);
}

BigInt* biAddBi(BigInt* a, const BigInt* b) {
    if (a->negative == b->negative) {
        add(a, b);
    } else {
        if (biCmpAbs(a, b) >= 0) {
            sub(a, a, b);
        } else {
            sub(a, b, a);
            a->negative = b->negative;
        }
    }
    if (a->mod) {
        biMod(a, a->mod);
    }
    if (biIsZero(a)) {
        a->negative = false;
    }
    return a;
}
BigInt* biAddInt(BigInt* a, I64 b_int) {
    BigInt b;
    U64 b_limbs[1];
    biInitInt(&b, b_limbs, b_int);

    return biAddBi(a, &b);
}

BigInt* biSubBi(BigInt* a, const BigInt* b) {
    if (a->negative != b->negative) {
        add(a, b);
    } else {
        if (biCmpAbs(a, b) >= 0) {
            sub(a, a, b);
        } else {
            sub(a, b, a);
            a->negative = !b->negative;
        }
    }
    if (a->mod) {
        biMod(a, a->mod);
    }
    if (biIsZero(a)) {
        a->negative = false;
    }
    return a;
}
BigInt* biSubInt(BigInt* a, I64 b_int) {
    BigInt b;
    U64 b_limbs[1];
    biInitInt(&b, b_limbs, b_int);

    return biSubBi(a, &b);
}

BigInt* biMulBi(BigInt* a, const BigInt* b) {
    if (biIsZero(a)) {
        return a;
    }
    if (biIsZero(b)) {
        clear(a, 1);
        a->length = 0;
        a->negative = false;
        return a;
    }

    U64 size = bitsToSize(a->length + b->length + 1);
    Allocator* temp_allocator;
    BigInt* temp_a;
    BigInt* temp_b;
    if (biCmpAbs(a, b) >= 0) {
        temp_allocator = allocatorNew((size + b->size) * sizeof(U64));
        temp_b = biClone(temp_allocator, b);
        temp_a = biClone(temp_allocator, a);
    } else {
        temp_allocator = allocatorNew((size + a->size) * sizeof(U64));
        temp_b = biClone(temp_allocator, a);
        temp_a = biClone(temp_allocator, b);
    }

    clear(a, size);
    a->negative = a->negative != b->negative;

    U64 limb = 0;
    U64 mask = 1;
    U64 shift = 0;
    while (limb < temp_b->size) {
        if (temp_b->limbs[limb] & mask) {
            biShiftLeft(temp_a, shift);
            shift = 0;
            add(a, temp_a);
        }
        shift++;
        mask <<= 1;
        if (!mask) {
            limb++;
            mask = 1;
        }
    }

    allocatorRelease(temp_allocator);

    if (a->mod) {
        biMod(a, a->mod);
    }

    return a;
}
BigInt* biMulInt(BigInt* a, I64 b_int) {
    BigInt b;
    U64 b_limbs[1];
    biInitInt(&b, b_limbs, b_int);

    return biMulBi(a, &b);
}

static void divide(Allocator* allocator, BigInt* a, const BigInt* b, BigInt* r) {
    biCopy(r, a);
    BigInt* orig_b = biClone(allocator, b);
    BigInt* temp_b = biClone(allocator, b);
    BigInt* mask = biFromInt(allocator, 1);

    biSetInt(a, 0);
    a->negative = a->negative != b->negative;

    U64 shift = 0;
    if (r->length > temp_b->length) {
        shift = r->length - b->length;
    }
    biShiftLeft(temp_b, shift);
    if (biCmpAbs(temp_b, r) < 0) {
        biShiftLeft(temp_b, 1);
        shift++;
    }
    biShiftLeft(mask, shift);

    while (biCmpAbs(r, orig_b) >= 0) {
        if (biCmpAbs(r, temp_b) >= 0) {
            add(a, mask);
            sub(r, r, temp_b);
        }
        shift = 1;
        if (temp_b->length > r->length) {
            shift = temp_b->length - r->length;
        }
        biShiftRight(temp_b, shift);
        biShiftRight(mask, shift);
    }

    if (a->mod) {
        biMod(a, a->mod);
    }
}

BigInt* biDivBi(BigInt* a, const BigInt* b) {
    PANIC_IF(biIsZero(b), "Divide by zero!");

    Allocator* temp_allocator = allocatorNew((a->size + (b->size * 4)) * sizeof(U64));
    divide(temp_allocator, a, b, biNew(temp_allocator));
    allocatorRelease(temp_allocator);

    return a;
}
BigInt* biDivInt(BigInt* a, I64 b_int) {
    BigInt b;
    U64 b_limbs[1];
    biInitInt(&b, b_limbs, b_int);

    return biDivBi(a, &b);
}

BigInt* biDivRemBi(BigInt* a, const BigInt* b, BigInt* r) {
    PANIC_IF(biIsZero(b), "Divide by zero!");

    Allocator* temp_allocator = allocatorNew((b->size * 4) * sizeof(U64));
    divide(temp_allocator, a, b, r);
    allocatorRelease(temp_allocator);

    return a;
}
BigInt* biDivRemInt(BigInt* a, I64 b_int, BigInt* r) {
    BigInt b;
    U64 b_limbs[1];
    biInitInt(&b, b_limbs, b_int);

    return biDivRemBi(a, &b, r);
}

BigInt* biModBi(BigInt* a, const BigInt* b) {
    PANIC_IF(biIsZero(b), "Divide by zero!");

    Allocator* temp_allocator = allocatorNew((b->size * 4) * sizeof(U64));
    BigInt* orig_b = biClone(temp_allocator, b);
    BigInt* temp_b = biClone(temp_allocator, b);
    BigInt* mask = biFromInt(temp_allocator, 1);

    U64 shift = 0;
    if (a->length > temp_b->length) {
        shift = a->length - b->length;
    }
    biShiftLeft(temp_b, shift);
    if (biCmpAbs(temp_b, a) < 0) {
        biShiftLeft(temp_b, 1);
        shift++;
    }
    biShiftLeft(mask, shift);

    while (biCmpAbs(a, orig_b) >= 0) {
        if (biCmpAbs(a, temp_b) >= 0) {
            sub(a, a, temp_b);
        }
        shift = 1;
        if (temp_b->length > a->length) {
            shift = temp_b->length - a->length;
        }
        biShiftRight(temp_b, shift);
        biShiftRight(mask, shift);
    }

    if (a->negative) {
        sub(a, b, a);
        a->negative = false;
    }

    allocatorRelease(temp_allocator);

    return a;
}
BigInt* biModInt(BigInt* a, I64 b_int) {
    BigInt b;
    U64 b_limbs[1];
    biInitInt(&b, b_limbs, b_int);

    return biModBi(a, &b);
}

BigInt* biPowBi(BigInt* a, const BigInt* b) {
    Allocator* temp_allocator = allocatorNew((a->size + b->size) * sizeof(U64));
    BigInt* temp_a = biClone(temp_allocator, a);
    BigInt* temp_b = biClone(temp_allocator, b);

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
                biMul(a, a);
                if (temp_b->limbs[limb] & mask) {
                    biMul(a, temp_a);
                }
            }
            mask >>= 1;
        }

    } while (limb>0);

    allocatorRelease(temp_allocator);

    return a;
}
BigInt* biPowInt(BigInt* a, I64 b_int) {
    BigInt b;
    U64 b_limbs[1];
    biInitInt(&b, b_limbs, b_int);

    return biPowBi(a, &b);
}

BigInt* biMulInv(BigInt* a, const BigInt* mod) {
    Allocator* temp_allocator = allocatorNew(a->size * 7 * sizeof(U64));

    BigInt* old_t = biFromInt(temp_allocator, 0);
    BigInt* t = biFromInt(temp_allocator, 1);
    BigInt* new_t = biNew(temp_allocator);

    BigInt* old_r = biClone(temp_allocator, mod);
    BigInt* r = biClone(temp_allocator, a);
    BigInt* new_r = biNew(temp_allocator);

    BigInt* q = biNew(temp_allocator);

    while (!biIsZero(r)) {
        biDivRem(biCopy(q, old_r), r, new_r);
        biSub(biCopy(new_t, old_t), biMul(q, t));

        biCopy(old_t, t);
        biCopy(t, new_t);
        biCopy(old_r, r);
        biCopy(r, new_r);
    }

    PANIC_IF(biCmp(old_r, 1) != 0, "GCD is not 1 so the inverse does not exsis\n");
    if (old_t->negative) {
        biAdd(old_t, mod);
    }
    biCopy(a, old_t);

    allocatorRelease(temp_allocator);

    return a;
}