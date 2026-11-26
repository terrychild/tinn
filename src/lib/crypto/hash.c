#include "lib/crypto/hash.h"
#include "lib/crypto/sha.h"
#include "lib/mem/slice.h"
#include "lib/log.h"

const Size hash_length[] = {
    [SHA_1] = 20,
    [SHA_2_224] = 28,
    [SHA_2_256] = 32,
    [SHA_2_384] = 48,
    [SHA_2_512] = 64,
};

const Size hash_block_length[] = {
    [SHA_1] = 64,
    [SHA_2_224] = 64,
    [SHA_2_256] = 64,
    [SHA_2_384] = 128,
    [SHA_2_512] = 128,
};

Slice hash(Memory* mem, HashAlgorithm hash, Slice source) {
    switch (hash) {
        case SHA_1:
            return sha_1(mem, source);
        case SHA_2_224:
            return sha_2_224(mem, source);
        case SHA_2_256:
            return sha_2_256(mem, source);
        case SHA_2_384:
            return sha_2_384(mem, source);
        case SHA_2_512:
            return sha_2_512(mem, source);
    }
    PANIC("Unknown hash algorithm.");
    return sliceEmpty();
}