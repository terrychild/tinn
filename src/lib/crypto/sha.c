#include <string.h>

#include "lib/bits.h"
#include "lib/bytes.h"

typedef void (*CompressionFn)(U32*, U8*);

static void md(U32* hash, U8* source, U64 source_len, U64 block_len, CompressionFn compression) {
    U8 block[block_len];

    // full blocks
    U64 read = 0;
    while (read + block_len < source_len) {
        memcpy(block, source + read, block_len);
        compression(hash, block);
        read += block_len;
    }

    // padded blocks
    int bytes_left = source_len - read;
    memcpy(block, source + read, bytes_left);
    memset(block + (bytes_left), 0x80, 1);

    int padding_bytes = block_len - bytes_left - 9;
    if (padding_bytes >= 0) {
        memset(block + (bytes_left + 1), 0, padding_bytes);
    } else {
        memset(block + (bytes_left + 1), 0, block_len - bytes_left - 1);
        compression(hash, block);
        memset(block, 0, 56);
    }
    intToBytes64(block + 56, source_len * 8);
    compression(hash, block);
}

static void sha1Compression(U32 hash[5], U8 source[64]) {
    // build schedule
    U32 w[80];
    for (int i=0; i<16; i++) {
        w[i] = bytesToInt32(source + (i*4));
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

bool sha1(U8 dest[20], U8* source, U64 len) {
    U32 hash[5] = {
        0x67452301,
        0xEFCDAB89,
        0x98BADCFE,
        0x10325476,
        0xC3D2E1F0
    };
    md(hash, source, len, 64, sha1Compression);

    for (int i=0; i<5; i++) {
        intToBytes32(dest + (i*4), hash[i]);
    }
    return true;
}

/*int sha_cmd(int argc, char* argv[]) {
    FILE *input_file;
    int fin_arg = arg(argc, argv, "--fin=");
    if (fin_arg<0) {
        input_file = stdin;
    } else {
        input_file = fopen(argv[fin_arg]+6, "w");
        if (input_file == NULL) {
            printf("unable to open file \"%s\"\n", argv[fin_arg]+6);
            return EXIT_FAILURE;
        }
    }

    FILE *output_file;
    int fout_arg = arg(argc, argv, "--fout=");
    if (fout_arg<0) {
        output_file = stdout;
    } else {
        output_file = fopen(argv[fout_arg]+7, "w");
        if (output_file == NULL) {
            printf("unable to open file \"%s\"\n", argv[fout_arg]+7);
            return EXIT_FAILURE;
        }
    }

    if (arg(argc, argv, "--hex")>=0) {
        char* hex = allocate(NULL, len*2);
        hex_chars(hex, buffer, len);
        fwrite(hex, 1, len*2, output_file);
        free(hex);
    } else {
        fwrite(buffer, 1, len, output_file);
    }
    if (fout_arg>=0) {
        fclose(output_file);
    }
    free(buffer);
    return EXIT_SUCCESS;
}*/