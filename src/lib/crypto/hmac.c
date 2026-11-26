#include "lib/crypto/hmac.h"
#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"

Slice hmac(Memory* mem, HashAlgorithm hash_algorithm, Slice key, Slice message) {
    ScratchMemory scratch = scratchMemory(mem);

    Size block_len = hash_block_length[hash_algorithm];
    if (key.length > block_len) {
        key = hash(scratch.mem, hash_algorithm, key);
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
    Slice inner_hash = hash(scratch.mem, hash_algorithm, bufAsSlice(inner_message));

    Buffer* outer_message = bufNew(scratch.mem, block_len + inner_hash.length);
    bufAppend(outer_message, outer_key, block_len);
    bufAppend(outer_message, inner_hash);
    Slice outer_hash = hash(mem, hash_algorithm, bufAsSlice(outer_message));

    scratchRelease(scratch);

    return outer_hash;
}