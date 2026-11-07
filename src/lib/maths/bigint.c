#include <stdbit.h>
#include <string.h>

#include "lib/maths/bigint.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/log.h"

static const U64 left_bit_mask = 0x8000000000000000;

// === declare some functions ===

static void add(BigInt* dest, const BigInt* a, const BigInt* b);
static void subtract(BigInt* dest, const BigInt* a, const BigInt* b);
static void multiply(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b);
static void divide(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b, BigInt* r);
static void modulus(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b);

// === size and length ===

static void resize(BigInt* bi, U64 new_size, bool clear_extra) {
    if (new_size > bi->size) {
        bi->size = new_size;
        bi->limbs = reallocate(bi->mem, bi->limbs, sizeof(bi->limbs) * bi->size);
    } else if (clear_extra && new_size < bi->size) {
        for (U64 limb = new_size; limb < bi->size; limb++) {
            bi->limbs[limb] = 0;
        }
    }
}

static void updateLen(BigInt* bi) {
    for (U64 limb = bi->size; limb-- > 0; ) {
        if (bi->limbs[limb]) {
            bi->length = (limb * 64) + 64 - stdc_leading_zeros_ull(bi->limbs[limb]);
            return;
        }
    }
    bi->length = 0;
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

BigInt* biNew(Memory* mem) {
    BigInt* bi = allocate(mem, sizeof(BigInt));
    bi->mem = mem;
    bi->size = 1;
    bi->length = 0;
    bi->negative = false;
    bi->limbs = allocate(mem, sizeof(bi->limbs) * bi->size);
    return bi;
}

BigInt* biFromBi(Memory* mem, const BigInt* source) {
    return biSetBi(biNew(mem), source);
}

BigInt* biFromInt(Memory* mem, I64 i) {
    return biSetInt(biNew(mem), i);
}

BigInt* biFromStr(Memory* mem, const char* str) {
    return biSetStr(biNew(mem), str);
}

// === input ===

BigInt* biSetBi(BigInt* dest, const BigInt* source) {
    resize(dest, source->size, true);
    for (U64 limb = 0; limb < dest->size; limb++) {
        dest->limbs[limb] = limb < source->size ? source->limbs[limb] : 0;
    }
    dest->length = source->length;
    dest->negative = source->negative;
    return dest;
}
BigInt* biSetInt(BigInt* bi, I64 i) {
    if (i>=0) {
        bi->limbs[0] = (U64)i;
        bi->negative = false;
    } else {
        bi->limbs[0] = (U64)(0-i);
        bi->negative = true;
    }
    for (U64 limb = 1; limb < bi->size; limb++) {
        bi->limbs[limb] = 0;
    }
    updateLen(bi);
    return bi;
}

static BigInt* biSetHexStr(BigInt* bi, const char* hex, U64 nibbles) {
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

    return bi;
}

static U8 readDecChar(char ch) {
	if (ch < '0') {
		ERROR("Invalid decimal char '%c'", ch);
		return 0;
	}
	U8 value = ch - '0';
	if (value > 9) {
		ERROR("Invalid decimal char '%c'", ch);
		return 0;
	}
	return value;
}

static BigInt* biSetDecStr(BigInt* bi, const char* dec, U64 digits) {
    U64 len = ((digits * 3.3219) + 31) / 32;

    resize(bi, len, true);

    ScratchMemory scratch = scratchMemory(bi->mem);
    BigInt* ten = biFromInt(scratch.mem, 10);
    BigInt* unit = biNew(scratch.mem);

    biSetInt(bi, 0);
    for (U64 i=0; i<digits; i++) {
        multiply(scratch.mem, bi, bi, ten);
        add(bi, bi, biSetInt(unit, readDecChar(dec[i])));
    }
    scratchRelease(scratch);

    return bi;
}

BigInt* biSetStr(BigInt* bi, const char* str) {
    U64 chars = strlen(str);
    if (chars == 0) {
        return biSetInt(bi, 0);
    }
    U64 start = str[0] == '-' ? 1 : 0;
    if (chars>start+2 && str[start] == '0' && str[start+1] == 'x') {
        biSetHexStr(bi, str+start+2, chars-start-2);
    } else {
        biSetDecStr(bi, str+start, chars-start);
    }
    if (start == 1) {
        bi->negative = true;
    }
    return bi;
}

// === output ===

Slice biToBESlice(Memory* mem, const BigInt* bi) {
    if (bi->length == 0) {
        return sliceNew(allocate(mem, 1), 1);
    }

    U64 bytes_len = ((bi->length - 1) / 8) + 1;

    U8* bytes = allocate(mem, bytes_len);

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

char* biToHexStr(Memory* mem, const BigInt* bi) {
    ScratchMemory scratch = scratchMemory(mem);
    Slice hex = biToBESlice(scratch.mem, bi);

    char* str = allocate(mem, (hex.length * 2) + 3);
    str[0] = '0';
    str[1] = 'x';
    bool go = false;
    U64 byte = 2;
    for (U64 i = 0; i < hex.length; i++) {
        if (!go && (hex.start[i] & 0xF0)) {
            go = true;
        }
        if (go) {
            str[byte++] = toHexChar((hex.start[i] & 0xF0) >> 4);
        }
        if (!go && (hex.start[i] & 0x0F)) {
            go = true;
        }
        if (go) {
            str[byte++] = toHexChar(hex.start[i] & 0x0F);
        }
    }
    str[byte] = '\0';

    scratchRelease(scratch);

    return str;
}

// === comparison ===

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

// === bit shifting ===

void shiftLeft(BigInt* bi, U64 count) {
    if (biIsZero(bi)) {
        return;
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
}

void shiftRight(BigInt* bi, U64 count) {
    if (bi->length <= count) {
        biSetInt(bi, 0);
        return;
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
}

// === addition ===

static void add(BigInt* dest, const BigInt* a, const BigInt* b) {
    U64 size = maxNormalSize(a, b);
    resize(dest, size, true);

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
static void biAdd_impl(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod) {
    if (a->negative == b->negative) {
        add(dest, a, b);
    } else {
        if (biCmpAbs(a, b) >= 0) {
            subtract(dest, a, b);
        } else {
            subtract(dest, b, a);
            dest->negative = b->negative;
        }
    }
    if (mod) {
        modulus(scratch_mem, dest, dest, mod);
    } else if (biIsZero(dest)) {
        dest->negative = false;
    }
}

BigInt* biAdd_bbb(BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biAdd_impl(scratch.mem, dest, a, b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biAdd_bbi(BigInt* dest, const BigInt* a, I64 b_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biAdd_impl(scratch.mem, dest, a, biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biAdd_bib(BigInt* dest, I64 a_int, const BigInt* b, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biAdd_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biAdd_bii(BigInt* dest, I64 a_int, I64 b_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biAdd_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}

BigInt* biAdd_mbb(Memory* mem, const BigInt* a, const BigInt* b, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biAdd_impl(scratch.mem, dest, a, b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biAdd_mbi(Memory* mem, const BigInt* a, I64 b_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biAdd_impl(scratch.mem, dest, a, biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biAdd_mib(Memory* mem, I64 a_int, const BigInt* b, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biAdd_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biAdd_mii(Memory* mem, I64 a_int, I64 b_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biAdd_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}

// === subtraction ===

static void subtract(BigInt* dest, const BigInt* a, const BigInt* b) {
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
static void biSub_impl(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod) {
    if (a->negative != b->negative) {
        add(dest, a, b);
    } else {
        if (biCmpAbs(a, b) >= 0) {
            subtract(dest, a, b);
        } else {
            subtract(dest, b, a);
            dest->negative = !b->negative;
        }
    }
    if (mod) {
        modulus(scratch_mem, dest, dest, mod);
    } else if (biIsZero(dest)) {
        dest->negative = false;
    }
}

BigInt* biSub_bbb(BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biSub_impl(scratch.mem, dest, a, b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biSub_bbi(BigInt* dest, const BigInt* a, I64 b_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biSub_impl(scratch.mem, dest, a, biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biSub_bib(BigInt* dest, I64 a_int, const BigInt* b, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biSub_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biSub_bii(BigInt* dest, I64 a_int, I64 b_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biSub_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}

BigInt* biSub_mbb(Memory* mem, const BigInt* a, const BigInt* b, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biSub_impl(scratch.mem, dest, a, b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biSub_mbi(Memory* mem, const BigInt* a, I64 b_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biSub_impl(scratch.mem, dest, a, biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biSub_mib(Memory* mem, I64 a_int, const BigInt* b, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biSub_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biSub_mii(Memory* mem, I64 a_int, I64 b_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biSub_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}

// === multiplication ===

static void multiply(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b) {
    BigInt* temp_b = biFromBi(scratch_mem, b);
    BigInt* temp_a = biFromBi(scratch_mem, a);

    biSetInt(dest, 0);
    dest->negative = a->negative != b->negative;

    U64 limb = 0;
    U64 mask = 1;
    U64 shift = 0;
    while (limb < temp_b->size) {
        if (temp_b->limbs[limb] & mask) {
            shiftLeft(temp_a, shift);
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
}
static void biMul_impl(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod) {
    if (biCmpAbs(a, b) >= 0) {
		multiply(scratch_mem, dest, a, b);
	} else {
        multiply(scratch_mem, dest, b, a);
    }

    if (mod) {
        modulus(scratch_mem, dest, dest, mod);
    }
}


BigInt* biMul_bbb(BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biMul_impl(scratch.mem, dest, a, b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMul_bbi(BigInt* dest, const BigInt* a, I64 b_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biMul_impl(scratch.mem, dest, a, biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMul_bib(BigInt* dest, I64 a_int, const BigInt* b, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biMul_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMul_bii(BigInt* dest, I64 a_int, I64 b_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biMul_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}

BigInt* biMul_mbb(Memory* mem, const BigInt* a, const BigInt* b, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biMul_impl(scratch.mem, dest, a, b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMul_mbi(Memory* mem, const BigInt* a, I64 b_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biMul_impl(scratch.mem, dest, a, biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMul_mib(Memory* mem, I64 a_int, const BigInt* b, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biMul_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMul_mii(Memory* mem, I64 a_int, I64 b_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biMul_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}

// === division ===

static void divide(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b, BigInt* r) {
    PANIC_IF(biIsZero(b), "Divide by zero!");

    biSet(r, a);
    BigInt* orig_b = biFromBi(scratch_mem, b);
    BigInt* temp_b = biFromBi(scratch_mem, b);
    BigInt* mask = biFromInt(scratch_mem, 1);

    biSetInt(dest, 0);
    dest->negative = a->negative != b->negative;

    U64 shift = 0;
    if (r->length > temp_b->length) {
        shift = r->length - b->length;
    }
    shiftLeft(temp_b, shift);
    if (biCmpAbs(temp_b, r) < 0) {
        shiftLeft(temp_b, 1);
        shift++;
    }
    shiftLeft(mask, shift);

    while (biCmpAbs(r, orig_b) >= 0) {
        if (biCmpAbs(r, temp_b) >= 0) {
            add(dest, dest, mask);
            subtract(r, r, temp_b);
        }
        shift = 1;
        if (temp_b->length > r->length) {
            shift = temp_b->length - r->length;
        }
        shiftRight(temp_b, shift);
        shiftRight(mask, shift);
    }
}
static void biDiv_impl(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b, BigInt* r) {
    divide(scratch_mem, dest, a, b, r == NULL ? biNew(scratch_mem) : r);
}

BigInt* biDiv_bbb(BigInt* dest, const BigInt* a, const BigInt* b, BigInt* r) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biDiv_impl(scratch.mem, dest, a, b, r);
    scratchRelease(scratch);
    return dest;
}
BigInt* biDiv_bbi(BigInt* dest, const BigInt* a, I64 b_int, BigInt* r) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biDiv_impl(scratch.mem, dest, a, biFrom(scratch.mem, b_int), r);
    scratchRelease(scratch);
    return dest;
}
BigInt* biDiv_bib(BigInt* dest, I64 a_int, const BigInt* b, BigInt* r) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biDiv_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), b, r);
    scratchRelease(scratch);
    return dest;
}
BigInt* biDiv_bii(BigInt* dest, I64 a_int, I64 b_int, BigInt* r) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    biDiv_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), r);
    scratchRelease(scratch);
    return dest;
}

BigInt* biDiv_mbb(Memory* mem, const BigInt* a, const BigInt* b, BigInt* r) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biDiv_impl(scratch.mem, dest, a, b, r);
    scratchRelease(scratch);
    return dest;
}
BigInt* biDiv_mbi(Memory* mem, const BigInt* a, I64 b_int, BigInt* r) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biDiv_impl(scratch.mem, dest, a, biFrom(scratch.mem, b_int), r);
    scratchRelease(scratch);
    return dest;
}
BigInt* biDiv_mib(Memory* mem, I64 a_int, const BigInt* b, BigInt* r) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biDiv_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), b, r);
    scratchRelease(scratch);
    return dest;
}
BigInt* biDiv_mii(Memory* mem, I64 a_int, I64 b_int, BigInt* r) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    biDiv_impl(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), r);
    scratchRelease(scratch);
    return dest;
}

// === modulus ===

void modulus(Memory* scratch_mem, BigInt* r, const BigInt* a, const BigInt* b) {
    PANIC_IF(biIsZero(b), "Divide by zero!");

    biSet(r, a);
    BigInt* orig_b = biFromBi(scratch_mem, b);
    BigInt* temp_b = biFromBi(scratch_mem, b);
    BigInt* mask = biFromInt(scratch_mem, 1);

    U64 shift = 0;
    if (r->length > temp_b->length) {
        shift = r->length - b->length;
    }
    shiftLeft(temp_b, shift);
    if (biCmpAbs(temp_b, r) < 0) {
        shiftLeft(temp_b, 1);
        shift++;
    }
    shiftLeft(mask, shift);

    while (biCmpAbs(r, orig_b) >= 0) {
        if (biCmpAbs(r, temp_b) >= 0) {
            subtract(r, r, temp_b);
        }
        shift = 1;
        if (temp_b->length > r->length) {
            shift = temp_b->length - r->length;
        }
        shiftRight(temp_b, shift);
        shiftRight(mask, shift);
    }

    if (r->negative) {
        subtract(r, b, r);
        r->negative = false;
    }
}

BigInt* biMod_bbb(BigInt* dest, const BigInt* a, const BigInt* b) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    modulus(scratch.mem, dest, a, b);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMod_bbi(BigInt* dest, const BigInt* a, I64 b_int) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    modulus(scratch.mem, dest, a, biFrom(scratch.mem, b_int));
    scratchRelease(scratch);
    return dest;
}
BigInt* biMod_bib(BigInt* dest, I64 a_int, const BigInt* b) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    modulus(scratch.mem, dest, biFrom(scratch.mem, a_int), b);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMod_bii(BigInt* dest, I64 a_int, I64 b_int) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    modulus(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int));
    scratchRelease(scratch);
    return dest;
}

BigInt* biMod_mbb(Memory* mem, const BigInt* a, const BigInt* b) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    modulus(scratch.mem, dest, a, b);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMod_mbi(Memory* mem, const BigInt* a, I64 b_int) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    modulus(scratch.mem, dest, a, biFrom(scratch.mem, b_int));
    scratchRelease(scratch);
    return dest;
}
BigInt* biMod_mib(Memory* mem, I64 a_int, const BigInt* b) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    modulus(scratch.mem, dest, biFrom(scratch.mem, a_int), b);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMod_mii(Memory* mem, I64 a_int, I64 b_int) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    modulus(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int));
    scratchRelease(scratch);
    return dest;
}

// === exponentiation ===

static void power(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod) {
    biSet(dest, a);
    BigInt* temp_a = biFromBi(scratch_mem, a);
    BigInt* temp_b = biFromBi(scratch_mem, b);

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
                multiply(scratch_mem, dest, dest, dest);
                if (mod) {
                    modulus(scratch_mem, dest, dest, mod);
                }
                if (temp_b->limbs[limb] & mask) {
                    multiply(scratch_mem, dest, dest, temp_a);
                    if (mod) {
                        modulus(scratch_mem, dest, dest, mod);
                    }
                }
            }
            mask >>= 1;
        }

    } while (limb>0);
}

BigInt* biPow_bbb(BigInt* dest, const BigInt* a, const BigInt* b, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    power(scratch.mem, dest, a, b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biPow_bbi(BigInt* dest, const BigInt* a, I64 b_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    power(scratch.mem, dest, a, biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biPow_bib(BigInt* dest, I64 a_int, const BigInt* b, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    power(scratch.mem, dest, biFrom(scratch.mem, a_int), b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biPow_bii(BigInt* dest, I64 a_int, I64 b_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    power(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}

BigInt* biPow_mbb(Memory* mem, const BigInt* a, const BigInt* b, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    power(scratch.mem, dest, a, b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biPow_mbi(Memory* mem, const BigInt* a, I64 b_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    power(scratch.mem, dest, a, biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biPow_mib(Memory* mem, I64 a_int, const BigInt* b, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    power(scratch.mem, dest, biFrom(scratch.mem, a_int), b, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biPow_mii(Memory* mem, I64 a_int, I64 b_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    power(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, b_int), mod);
    scratchRelease(scratch);
    return dest;
}

// === multiplicative inverse ===

static void mulInv(Memory* scratch_mem, BigInt* dest, const BigInt* a, const BigInt* mod) {
    BigInt* old_t = biFromInt(scratch_mem, 0);
    BigInt* t = biFromInt(scratch_mem, 1);
    BigInt* new_t = biNew(scratch_mem);

    BigInt* old_r = biFromBi(scratch_mem, mod);
    BigInt* r = biFromBi(scratch_mem, a);
    BigInt* new_r = biNew(scratch_mem);

    BigInt* q = biNew(scratch_mem);

    while (!biIsZero(r)) {
        divide(scratch_mem, q, old_r, r, new_r);
        multiply(scratch_mem, new_t, q, t);
        biSub_bbb(new_t, old_t, new_t, mod);

        biSet(old_t, t);
        biSet(t, new_t);
        biSet(old_r, r);
        biSet(r, new_r);
    }

    PANIC_IF(biCmp(old_r, 1) != 0, "GCD is not 1 so the inverse does not exsis\n");

    biSet(dest, old_t);
    if (dest->negative) {
        add(dest, dest, mod);
    }
}

BigInt* biMulInv_bbb(BigInt* dest, const BigInt* a, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    mulInv(scratch.mem, dest, a, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMulInv_bbi(BigInt* dest, const BigInt* a, I64 mod_int) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    mulInv(scratch.mem, dest, a, biFrom(scratch.mem, mod_int));
    scratchRelease(scratch);
    return dest;
}
BigInt* biMulInv_bib(BigInt* dest, I64 a_int, const BigInt* mod) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    mulInv(scratch.mem, dest, biFrom(scratch.mem, a_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMulInv_bii(BigInt* dest, I64 a_int, I64 mod_int) {
    ScratchMemory scratch = scratchMemory(dest->mem);
    mulInv(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, mod_int));
    scratchRelease(scratch);
    return dest;
}

BigInt* biMulInv_mbb(Memory* mem, const BigInt* a, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    mulInv(scratch.mem, dest, a, mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMulInv_mbi(Memory* mem, const BigInt* a, I64 mod_int) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    mulInv(scratch.mem, dest, a, biFrom(scratch.mem, mod_int));
    scratchRelease(scratch);
    return dest;
}
BigInt* biMulInv_mib(Memory* mem, I64 a_int, const BigInt* mod) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    mulInv(scratch.mem, dest, biFrom(scratch.mem, a_int), mod);
    scratchRelease(scratch);
    return dest;
}
BigInt* biMulInv_mii(Memory* mem, I64 a_int, I64 mod_int) {
    BigInt* dest = biNew(mem);
    ScratchMemory scratch = scratchMemory(mem);
    mulInv(scratch.mem, dest, biFrom(scratch.mem, a_int), biFrom(scratch.mem, mod_int));
    scratchRelease(scratch);
    return dest;
}