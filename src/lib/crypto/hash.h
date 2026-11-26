#ifndef LIB_CRYPTO_HASH_H
#define LIB_CRYPTO_HASH_H

#include "lib/types.h"

typedef enum {
    SHA_1,
    SHA_2_224,
    SHA_2_256,
    SHA_2_384,
    SHA_2_512
} HashAlgorithm;

extern const Size hash_length[];
extern const Size hash_block_length[];

Slice hash(Memory* mem, HashAlgorithm hash, Slice source);

#endif