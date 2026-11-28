#include <string.h>

#include "lib/crypto/aes.h"
#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"
#include "lib/bits.h"
#include "lib/bytes.h"
#include "lib/log.h"

#include "lib/mem/slice.h"

// Perform AES multiplication, i.e. polynomial multiplication in the field
// GF(256) with the irreducible polynomial x^8 + x^4 + x^3 + x + 1.  Designed
// to work with 8 bit numbers despite the irreducible polynomial requiring 9
// bits.
static U8 aes_mul(U8 a, U8 b) {
    U8 result = 0;

    for (int i=0; i<8; i++) {
        if (b & 0x01) {
            result ^= a;
        }
        a = (a << 1) ^ (a & 0x80 ? 0x1b : 0);
        b >>= 1;
    }

    return result;
}

// Perform polynomial division with 8 bit vectors.
static U8 poly_div(U8 a, U8 b, U8* r) {
    U8 mask = 0x80;
    while (mask && !(mask & a) && !(mask & b)) {
        mask >>= 1;
    }

    if (!(mask & a)) {
        *r = a;
        return 0;
    }

    U8 mag = 0;
    while (mask && !(mask & b)) {
        mask >>= 1;
        mag++;
    }

    return (1 << mag) ^ poly_div(a ^ (b << mag), b, r);
}
// Perform polynomial division with the AES irreducible polynomial (which
// would normally require 9 bits) with an 8 bit vector.
static U8 aes_div(U8 b, U8* r) {
    U8 mask = 0x80;
    U8 mag = 1;
    while (mask && !(mask & b)) {
        mask >>= 1;
        mag++;
    }

    return (1 << mag) ^ poly_div(0x1b ^ (b << mag), b, r);
}

// Compute the AES multiplicative inverse using the Extended Euclidean Algorithm
static U8 aes_inverse(U8 r) {
    U8 old_t = 0;
    U8 t = 1;

    U8 new_r;
    U8 q = aes_div(r, &new_r);

    while (new_r != 0) {
        U8 new_t = old_t ^ aes_mul(q, t);
        old_t = t;
        t = new_t;

        U8 old_r = r;
        r = new_r;
        q = poly_div(old_r, r, &new_r);
    }

    return t;
}

// AES S-box values
static U8 forward_sbox[256] = {};
static U8 inverse_sbox[256] = {};

static void compute_sbox() {
    forward_sbox[0] = 0x63;
    inverse_sbox[0x63] = 0;

    U8 value = 0;
    do {
        U8 inverse = aes_inverse(++value);
        forward_sbox[value] = inverse ^ rotl8(inverse, 1) ^ rotl8(inverse, 2) ^ rotl8(inverse, 3) ^ rotl8(inverse, 4) ^ 0x63;
        inverse_sbox[forward_sbox[value]] = value;
    } while (value != 255);
}

// Key expansion
static void copy_word(U8 dest[4], const U8 src[4]) {
    for (int i=0; i<4; i++) {
        dest[i] = src[i];
    }
}
static void add_word(U8 dest[4], const U8 word[4]) {
    for (int i=0; i<4; i++) {
        dest[i] ^= word[i];
    }
}
static void rotate_word(U8 word[4]) {
    U8 temp = word[0];
    for (int i=0; i<3; i++) {
        word[i] = word[i+1];
    }
    word[3] = temp;
}
static void sub_word(U8 word[4]) {
    for (int i=0; i<4; i++) {
        word[i] = forward_sbox[word[i]];
    }
}

static U8* expand_key(Memory* mem, const U8* initial_key, Size starting_words, Size total_words) {
    U8* key = allocate(mem, total_words * 4);

    // initial key
    for (Size i=0; i<starting_words; i++) {
        copy_word(key + (i*4), initial_key + (i*4));
    }

    // expand
    U8 rc = 0x01;
    for (Size i=starting_words; i<total_words; i++) {
        U8* current_word = key + (i*4);
        U8* prev_word = key + ((i-1)*4);
        U8* prev_round = key + ((i-starting_words)*4);

        copy_word(current_word, prev_word);
        if (i%starting_words == 0) {
            rotate_word(current_word);
            sub_word(current_word);
            current_word[0] ^= rc;
            rc = aes_mul(rc, 0x02);
        } else if (starting_words == 8 && i%4 == 0) {
            sub_word(current_word);
        }
        add_word(current_word, prev_round);
    }

    return key;
}

// encryption/decryption
static void add_blocks(U8 a[16], U8 b[16]) {
    for (int i=0; i<16; i++) {
        a[i] ^= b[i];
    }
}
static void sub_bytes(U8 block[16], U8 sbox[256]) {
    for (int i=0; i<16; i++) {
        block[i] = sbox[block[i]];
    }
}

static void shift_rows(U8 block[16]) {
    // row 2
    U8 temp = block[1];
    block[1] = block[5];
    block[5] = block[9];
    block[9] = block[13];
    block[13] = temp;

    // row 3
    temp = block[2];
    block[2] = block[10];
    block[10] = temp;
    temp = block[6];
    block[6] = block[14];
    block[14] = temp;

    // row 4
    temp = block[15];
    block[15] = block[11];
    block[11] = block[7];
    block[7] = block[3];
    block[3] = temp;
}
static void inverse_shift_rows(U8 block[16]) {
    // row 2
    U8 temp = block[13];
    block[13] = block[9];
    block[9] = block[5];
    block[5] = block[1];
    block[1] = temp;

    // row 3
    temp = block[2];
    block[2] = block[10];
    block[10] = temp;
    temp = block[6];
    block[6] = block[14];
    block[14] = temp;

    // row 4
    temp = block[3];
    block[3] = block[7];
    block[7] = block[11];
    block[11] = block[15];
    block[15] = temp;
}

static U8 columns_matrix[4][4] = {
    {2, 3, 1, 1},
    {1, 2, 3, 1},
    {1, 1, 2, 3},
    {3, 1, 1, 2}
};
static U8 inverse_columns_matrix[4][4] = {
    {14, 11, 13, 9},
    {9, 14, 11, 13},
    {13, 9, 14, 11},
    {11, 13, 9, 14}
};
static void mix_column(U8 block[4], U8 matrix[4][4]) {
    U8 result[4];

    for (int h=0; h<4; h++) {
        result[h] = 0;
        for (int w=0; w<4; w++) {
            result[h] ^= aes_mul(matrix[h][w], block[w]);
        }
    }

    for (int i=0; i<4; i++) {
        block[i] = result[i];
    }
}
static void mix_columns(U8 block[16], U8 matrix[4][4]) {
    for (int i=0; i<4; i++) {
        mix_column(block + (i*4), matrix);
    }
}

static void encrypt(U8 block[16], int rounds, U8* key) {
    add_blocks(block, key);
    for (int round=1; round<=rounds; round++) {
        sub_bytes(block, forward_sbox);
        shift_rows(block);
        if (round<rounds) {
            mix_columns(block, columns_matrix);
        }
        add_blocks(block, key+(round*16));
    }
}

static void decrypt(U8 block[16], int rounds, U8* key) {
    for (int round=rounds; round>=1; round--) {
        add_blocks(block, key+(round*16));
        if (round<rounds) {
            mix_columns(block, inverse_columns_matrix);
        }
        inverse_shift_rows(block);
        sub_bytes(block, inverse_sbox);
    }
    add_blocks(block, key);
}

// modes of opperation
static void tag_mul(U8 a[16], U8 b[16]) {
    U8 result[16] = {};
    U8 mul[16];
    memcpy(mul, b, 16);

    for (int i=0; i<16; i++) {
        U8 mask = 0x80;
        for (int j=0; j<8; j++) {
            if (a[i] & mask) {
                for (int k=0; k<16; k++) {
                    result[k] ^= mul[k];
                }
            }
            mask >>= 1;

            U8 carry = 0;
            for (int k=0; k<16; k++) {
                U8 next_carry = mul[k] & 0x01;
                mul[k] = (mul[k] >> 1) | carry;
                carry = next_carry ? 0x80 : 0;
            }
            if (carry) {
                mul[0] ^= 0xE1;
            }
        }
    }

    memcpy(a, result, 16);
}

Slice aes_gcs_encrypt(Memory* mem, Slice key, Slice iv, Slice plain_text, Slice additional_data) {
    PANIC_IF(key.length != 16 && key.length != 24 && key.length != 32, "AES: Invalid key length.");
    PANIC_IF(iv.length != 12, "AES: Invalid IV length");

    // compute s-boxes if required
    if (forward_sbox[0] == 0) {
        compute_sbox();
    }

    ScratchMemory scratch = scratchMemory(mem);

    Size initial_key_words = key.length / 4;
    Size rounds = initial_key_words + 6;
    Size total_key_words = (rounds + 1) * 4;
    U8* expanded_key = expand_key(scratch.mem, key.start, initial_key_words, total_key_words);

    U8 block[16];
    U8 hash[16] = {};
    encrypt(hash, rounds, expanded_key);
    U8 tag[16] = {};

    // add additional data
    for (Size i=0; i*16 < additional_data.length; i++) {
        for (U8 j=0; j<16 && j+(i*16) < additional_data.length; j++) {
            tag[j] ^= additional_data.start[j+(i*16)];
        }
        tag_mul(tag, hash);
    }

    // do the encryption
    Buffer* cipher_text = bufNew(mem, plain_text.length);
    for (U32 i=0; i*16 < plain_text.length; i++) {
        memcpy(block, iv.start, 12);
        toBE32(block + 12, i+2);
        encrypt(block, rounds, expanded_key);

        for (U8 j=0; j<16 && j+(i*16) < plain_text.length; j++) {
            U8 cipher_byte = plain_text.start[j+(i*16)] ^ block[j];
            bufAppendBE8(cipher_text, cipher_byte);
            tag[j] ^= cipher_byte;
        }
        tag_mul(tag, hash);
    }

    // add lengths to tag
    toBE64(block, additional_data.length * 8);
    toBE64(block + 8, plain_text.length * 8);
    for (U8 j=0; j<16; j++) {
        tag[j] ^= block[j];
    }
    tag_mul(tag, hash);

    // add IV to tag
    memcpy(block, iv.start, 12);
    toBE32(block + 12, 1);
    encrypt(block, rounds, expanded_key);
    for (U8 j=0; j<16; j++) {
        tag[j] ^= block[j];
    }

    // add tag to output
    bufAppend(cipher_text, tag, 16);

    scratchRelease(scratch);

    return bufAsSlice(cipher_text);
}