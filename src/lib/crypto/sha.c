#include <stdlib.h>
#include <string.h>

#include "lib/crypto/sha.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/bits.h"
#include "lib/bytes.h"

typedef void (*compression_32)(U32*, U8*);

static void md_32(U32* hash, Slice source, U64 block_len, compression_32 compression) {
    ScratchMemory scratch = scratchMemory(NULL);
    U8* block = allocate(scratch.mem, block_len);

    // full blocks
    U64 bytes_read = 0;
    while (bytes_read + block_len < source.length) {
        memcpy(block, source.start + bytes_read, block_len);
        compression(hash, block);
        bytes_read += block_len;
    }

    // padded blocks
    int bytes_left = source.length - bytes_read;
    memcpy(block, source.start + bytes_read, bytes_left);
    memset(block + (bytes_left), 0x80, 1);

    int padding_bytes = block_len - bytes_left - 9;
    if (padding_bytes >= 0) {
        memset(block + (bytes_left + 1), 0, padding_bytes);
    } else {
        memset(block + (bytes_left + 1), 0, block_len - bytes_left - 1);
        compression(hash, block);
        memset(block, 0, 56);
    }
    toBE64(block + 56, source.length * 8);
    compression(hash, block);

    scratchRelease(scratch);
}

static void sha1Compression(U32 hash[5], U8 source[64]) {
    // build schedule
    U32 w[80];
    for (int i=0; i<16; i++) {
        w[i] = fromBE32(source + (i*4));
    }
    for (int i=16; i<80; i++) {
        w[i] = rotl32((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);
    }

    // read hash
    U32 a = hash[0];
    U32 b = hash[1];
    U32 c = hash[2];
    U32 d = hash[3];
    U32 e = hash[4];

    // main loop
    for (int i=0; i<80; i++) {
        U32 f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) ^ (b & d) ^ (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        U32 temp = rotl32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotl32(b, 30);
        b = a;
        a = temp;
    }

    // export hash
    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
}

static U32 sha_256_k[] = {
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void sha256Compression(U32 hash[8], U8 source[64]) {
    // build schedule
    U32 w[64];
    for (int i=0; i<16; i++) {
        w[i] = fromBE32(source + (i*4));
    }
    for (int i=16; i<64; i++) {
        U32 s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3);
        U32 s1 = rotr32(w[i-2], 17) ^ rotr32(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    // read hash
    U32 a = hash[0];
    U32 b = hash[1];
    U32 c = hash[2];
    U32 d = hash[3];
    U32 e = hash[4];
    U32 f = hash[5];
    U32 g = hash[6];
    U32 h = hash[7];

    // main loop
    for (int i=0; i<64; i++) {
        U32 temp0 = h + sha_256_k[i] + w[i];
        temp0 += (e & f) ^ (~e & g);
        temp0 += rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);

        U32 temp1 = temp0;
        temp1 += (a & b) ^ (a & c) ^ (b & c);
        temp1 += rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);

        h = g;
        g = f;
        f = e;
        e = d + temp0;
        d = c;
        c = b;
        b = a;
        a = temp1;
    }

    // export hash
    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
    hash[5] += f;
    hash[6] += g;
    hash[7] += h;
}

Slice sha1(Memory* mem, Slice source) {
    U32 hash[5] = { 0x67452301,	0xEFCDAB89,	0x98BADCFE,	0x10325476,	0xC3D2E1F0 };
    md_32(hash, source, 64, sha1Compression);

    U8* dest = allocate(mem, 20);
    for (int i=0; i<5; i++) {
        toBE32(dest + (i*4), hash[i]);
    }
    return sliceNew(dest, 20);
}

Slice sha224(Memory* mem, Slice source) {
    U32 hash[8] = { 0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939, 0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4 };
    md_32(hash, source, 64, sha256Compression);

    U8* dest = allocate(mem, 28);
    for (int i=0; i<7; i++) {
        toBE32(dest + (i*4), hash[i]);
    }
    return sliceNew(dest, 28);
}

Slice sha256(Memory* mem, Slice source) {
    U32 hash[8] = { 0x6a09e667,	0xbb67ae85,	0x3c6ef372,	0xa54ff53a,	0x510e527f,	0x9b05688c,	0x1f83d9ab,	0x5be0cd19 };
    md_32(hash, source, 64, sha256Compression);

    U8* dest = allocate(mem, 32);
    for (int i=0; i<8; i++) {
        toBE32(dest + (i*4), hash[i]);
    }
    return sliceNew(dest, 32);
}