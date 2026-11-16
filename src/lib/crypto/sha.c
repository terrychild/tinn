#include <string.h>

#include "lib/crypto/sha.h"
#include "lib/mem/memory.h"
#include "lib/mem/slice.h"
#include "lib/bits.h"
#include "lib/bytes.h"
#include "lib/maths/minmax.h"
#include "lib/maths/bigint.h"

typedef void (*Compression_32)(U32*, const U8*);
typedef void (*Compression_64)(U64*, const U8*);
typedef Slice (*Sha)(Memory* mem, const Slice* source, Size source_count);

static void md_32(U32* hash, const Slice* source, Size source_count, Size block_len, Compression_32 compression) {
    ScratchMemory scratch = scratchMemory(NULL);
    U8* block = allocate(scratch.mem, block_len);

    // data
    Size total_bytes = 0;
    Size bytes_read = 0;
    Size bytes_written = 0;
    Size i = 0;
    while (i < source_count) {
        Size bytes_to_write = min(source[i].length - bytes_read, block_len - bytes_written);
        memcpy(block + bytes_written, source[i].start + bytes_read, bytes_to_write);
        bytes_read += bytes_to_write;
        bytes_written += bytes_to_write;
        if (bytes_written == block_len) {
            compression(hash, block);
            bytes_written = 0;
        }
        if (bytes_read == source[i].length) {
            total_bytes += bytes_read;
            bytes_read = 0;
            i += 1;
        }
    }

    // padding
    memset(block + bytes_written, 0x80, 1);
    int padding_bytes = block_len - bytes_written - 9;
    if (padding_bytes >= 0) {
        memset(block + (bytes_written + 1), 0, padding_bytes);
    } else {
        memset(block + (bytes_written + 1), 0, block_len - bytes_written - 1);
        compression(hash, block);
        memset(block, 0, block_len - 8);
    }
    toBE64(block + block_len - 8, total_bytes * 8);
    compression(hash, block);

    scratchRelease(scratch);
}

static void md_64(U64* hash, const Slice* source, Size source_count, Size block_len, Compression_64 compression) {
    ScratchMemory scratch = scratchMemory(NULL);
    U8* block = allocate(scratch.mem, block_len);

    // full blocks
    Size total_bytes = 0;
    Size bytes_read = 0;
    Size bytes_written = 0;
    Size i = 0;
    while (i < source_count) {
        Size bytes_to_write = min(source[i].length - bytes_read, block_len - bytes_written);
        memcpy(block + bytes_written, source[i].start + bytes_read, bytes_to_write);
        bytes_read += bytes_to_write;
        bytes_written += bytes_to_write;
        if (bytes_written == block_len) {
            compression(hash, block);
            bytes_written = 0;
        }
        if (bytes_read == source[i].length) {
            total_bytes += bytes_read;
            bytes_read = 0;
            i += 1;
        }
    }

    // padding
    memset(block + bytes_written, 0x80, 1);
    int padding_bytes = block_len - bytes_written - 17;
    if (padding_bytes >= 0) {
        memset(block + (bytes_written + 1), 0, padding_bytes);
    } else {
        memset(block + (bytes_written + 1), 0, block_len - bytes_written - 1);
        compression(hash, block);
        memset(block, 0, block_len - 16);
    }
    toBE128(block + block_len - 16, biMul(scratch.mem, total_bytes, 8));
    compression(hash, block);

    scratchRelease(scratch);
}

static void sha1Compression(U32 hash[5], const U8 source[64]) {
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

static void sha256Compression(U32 hash[8], const U8 source[64]) {
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

static U64 sha_512_k[] = {
    0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc, 0x3956c25bf348b538,
    0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242, 0x12835b0145706fbe,
    0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2, 0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235,
    0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
    0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5, 0x983e5152ee66dfab,
    0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725,
    0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed,
    0x53380d139d95b3df, 0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
    0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218,
    0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8, 0x19a4c116b8d2d0c8, 0x1e376c085141ab53,
    0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373,
    0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
    0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b, 0xca273eceea26619c,
    0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba, 0x0a637dc5a2c898a6,
    0x113f9804bef90dae, 0x1b710b35131c471b, 0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc,
    0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
};

static void sha512Compression(U64 hash[8], const U8 source[128]) {
    // build schedule
    U64 w[80];
    for (int i=0; i<16; i++) {
        w[i] = fromBE64(source + (i*8));
    }
    for (int i=16; i<80; i++) {
        U64 s0 = rotr64(w[i-15], 1) ^ rotr64(w[i-15], 8) ^ (w[i-15] >> 7);
        U64 s1 = rotr64(w[i-2], 19) ^ rotr64(w[i-2], 61) ^ (w[i-2] >> 6);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    // read hash
    U64 a = hash[0];
    U64 b = hash[1];
    U64 c = hash[2];
    U64 d = hash[3];
    U64 e = hash[4];
    U64 f = hash[5];
    U64 g = hash[6];
    U64 h = hash[7];

    // main loop
    for (int i=0; i<80; i++) {
        U64 temp0 = h + sha_512_k[i] + w[i];
        temp0 += (e & f) ^ (~e & g);
        temp0 += rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);

        U64 temp1 = temp0;
        temp1 += (a & b) ^ (a & c) ^ (b & c);
        temp1 += rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);

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

Slice sha1_Slices(Memory* mem, const Slice* source, Size source_count) {
    U32 hash[5] = { 0x67452301,	0xEFCDAB89,	0x98BADCFE,	0x10325476,	0xC3D2E1F0 };
    md_32(hash, source, source_count, 64, sha1Compression);

    U8* dest = allocate(mem, 20);
    for (int i=0; i<5; i++) {
        toBE32(dest + (i*4), hash[i]);
    }
    return sliceNew(dest, 20);
}
Slice sha1_Slice(Memory* mem, const Slice source) {
    return sha1_Slices(mem, &source, 1);
}

Slice sha224_Slices(Memory* mem, const Slice* source, Size source_count) {
    U32 hash[8] = { 0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939, 0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4 };
    md_32(hash, source, source_count, 64, sha256Compression);

    U8* dest = allocate(mem, 28);
    for (int i=0; i<7; i++) {
        toBE32(dest + (i*4), hash[i]);
    }
    return sliceNew(dest, 28);
}
Slice sha224_Slice(Memory* mem, const Slice source) {
    return sha224_Slices(mem, &source, 1);
}

Slice sha256_Slices(Memory* mem, const Slice* source, Size source_count) {
    U32 hash[8] = { 0x6a09e667,	0xbb67ae85,	0x3c6ef372,	0xa54ff53a,	0x510e527f,	0x9b05688c,	0x1f83d9ab,	0x5be0cd19 };
    md_32(hash, source, source_count, 64, sha256Compression);

    U8* dest = allocate(mem, 32);
    for (int i=0; i<8; i++) {
        toBE32(dest + (i*4), hash[i]);
    }
    return sliceNew(dest, 32);
}
Slice sha256_Slice(Memory* mem, const Slice source) {
    return sha256_Slices(mem, &source, 1);
}

Slice sha384_Slices(Memory* mem, const Slice* source, Size source_count) {
    U64 hash[8] = {
        0xcbbb9d5dc1059ed8, 0x629a292a367cd507, 0x9159015a3070dd17, 0x152fecd8f70e5939,
        0x67332667ffc00b31, 0x8eb44a8768581511, 0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4
    };
    md_64(hash, source, source_count, 128, sha512Compression);

    U8* dest = allocate(mem, 48);
    for (int i=0; i<6; i++) {
        toBE64(dest + (i*8), hash[i]);
    }
    return sliceNew(dest, 48);
}
Slice sha384_Slice(Memory* mem, const Slice source) {
    return sha384_Slices(mem, &source, 1);
}

Slice sha512_Slices(Memory* mem, const Slice* source, Size source_count) {
    U64 hash[8] = {
        0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
        0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
    };
    md_64(hash, source, source_count, 128, sha512Compression);

    U8* dest = allocate(mem, 64);
    for (int i=0; i<8; i++) {
        toBE64(dest + (i*8), hash[i]);
    }
    return sliceNew(dest, 64);
}
Slice sha512_Slice(Memory* mem, const Slice source) {
    return sha512_Slices(mem, &source, 1);
}

static Slice hmacSha(Memory* mem, Slice key, const Slice* source, Size source_count, Size block_len, Sha sha) {
    ScratchMemory scratch = scratchMemory(mem);

    if (key.length > block_len) {
        key = sha(scratch.mem, &key, 1);
    }
    U8 inner_key[block_len];
    U8 outer_key[block_len];
    for (Size i=0; i<block_len; i++) {
        inner_key[i] = i < key.length ? key.start[i] ^ 0x36 : 0x36;
        outer_key[i] = i < key.length ? key.start[i] ^ 0x5c : 0x5c;
    }
    Slice* inner_slices = allocate(scratch.mem, sizeof(Slice) * (source_count + 1));
    inner_slices[0].length = block_len;
    inner_slices[0].start = inner_key;
    memcpy(inner_slices + 1, source, sizeof(Slice) * source_count);
    Slice inner_hash = sha(scratch.mem, inner_slices, source_count + 1);
    Slice outer_hash = sha(mem, ((Slice[]){sliceNew(outer_key, block_len), inner_hash}), 2);

    scratchRelease(scratch);

    return outer_hash;
}

Slice hmacSha224_Slices(Memory* mem, const Slice key, const Slice* source, Size source_count) {
    return hmacSha(mem, key, source, source_count, 64, sha224_Slices);
}
Slice hmacSha224_Slice(Memory* mem, const Slice key, const Slice source) {
    return hmacSha(mem, key, &source, 1, 64, sha224_Slices);
}

Slice hmacSha256_Slices(Memory* mem, const Slice key, const Slice* source, Size source_count) {
    return hmacSha(mem, key, source, source_count, 64, sha256_Slices);
}
Slice hmacSha256_Slice(Memory* mem, const Slice key, const Slice source) {
    return hmacSha(mem, key, &source, 1, 64, sha256_Slices);
}

Slice hmacSha384_Slices(Memory* mem, const Slice key, const Slice* source, Size source_count) {
    return hmacSha(mem, key, source, source_count, 128, sha384_Slices);
}
Slice hmacSha384_Slice(Memory* mem, const Slice key, const Slice source) {
    return hmacSha(mem, key, &source, 1, 128, sha384_Slices);
}

Slice hmacSha512_Slices(Memory* mem, const Slice key, const Slice* source, Size source_count) {
    return hmacSha(mem, key, source, source_count, 128, sha512_Slices);
}
Slice hmacSha512_Slice(Memory* mem, const Slice key, const Slice source) {
    return hmacSha(mem, key, &source, 1, 128, sha512_Slices);
}
