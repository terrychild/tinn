#include "lib/crypto/hash.h"

const Size hash_length[] = {
    [SHA_1] = 20,
    [SHA_2_224] = 28,
    [SHA_2_256] = 32,
    [SHA_2_384] = 48,
    [SHA_2_512] = 64,
};