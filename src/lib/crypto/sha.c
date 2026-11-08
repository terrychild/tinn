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

static void sha_1_compression(U32 hash[5], U8 source[64]) {
	// build schedule
	U32 w[80];
	for (int i=0; i<16; i++) {
		w[i] = fromBE32(source + (i*4));
	}
	for (int i=16; i<80; i++) {
        w[i] = rotl32((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);
	}

	// main loop
	U32 a = hash[0];
	U32 b = hash[1];
	U32 c = hash[2];
	U32 d = hash[3];
	U32 e = hash[4];

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
	hash[0] = hash[0] + a;
	hash[1] = hash[1] + b;
	hash[2] = hash[2] + c;
	hash[3] = hash[3] + d;
	hash[4] = hash[4] + e;
}

Slice sha1(Memory* mem, Slice source) {
	U32 hash[5] = {
		0x67452301,
		0xEFCDAB89,
		0x98BADCFE,
		0x10325476,
		0xC3D2E1F0
	};
	md_32(hash, source, 64, sha_1_compression);

    U8* dest = allocate(mem, 20);
	for (int i=0; i<5; i++) {
		toBE32(dest + (i*4), hash[i]);
	}
    return sliceNew(dest, 20);
}