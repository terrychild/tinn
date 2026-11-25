#include "lib/crypto/hmac.h"
#include "lib/crypto/sha.h"
#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"

typedef Slice (*HashFunc)(Memory* mem, Slice message);

static Slice hmacImpl(Memory* mem, Slice key, Slice message, Size block_len, HashFunc hash_func) {
    ScratchMemory scratch = scratchMemory(mem);

    if (key.length > block_len) {
        key = hash_func(scratch.mem, key);
    }
    U8 inner_key[block_len];
    U8 outer_key[block_len];
    for (Size i=0; i<block_len; i++) {
        inner_key[i] = i < key.length ? key.start[i] ^ 0x36 : 0x36;
        outer_key[i] = i < key.length ? key.start[i] ^ 0x5c : 0x5c;
    }

    Buffer* inner_message = bufNew(scratch.mem, block_len + message.length);
    bufAppend(inner_message, inner_key, block_len);
    bufAppend(inner_message, message);
    Slice inner_hash = hash_func(scratch.mem, bufAsSlice(inner_message));

    Buffer* outer_message = bufNew(scratch.mem, block_len + inner_hash.length);
    bufAppend(outer_message, outer_key, block_len);
    bufAppend(outer_message, inner_hash);
    Slice outer_hash = hash_func(mem, bufAsSlice(outer_message));

    scratchRelease(scratch);

    return outer_hash;
}

Slice hmac(Memory* mem, HashAlgorithm hash, Slice key, Slice message) {
    switch (hash) {
        case SHA_1:
            return hmacImpl(mem, key, message, 64, sha_1);
        case SHA_2_224:
            return hmacImpl(mem, key, message, 64, sha_2_224);
        case SHA_2_256:
            return hmacImpl(mem, key, message, 64, sha_2_256);
        case SHA_2_384:
            return hmacImpl(mem, key, message, 128, sha_2_384);
        case SHA_2_512:
            return hmacImpl(mem, key, message, 128, sha_2_512);
    }
}