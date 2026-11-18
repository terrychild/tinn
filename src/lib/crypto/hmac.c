#include "lib/crypto/hmac.h"
#include "lib/crypto/sha.h"
#include "lib/mem/memory.h"
#include "lib/mem/reader.h"

typedef Slice (*HashFunc)(Memory* mem, Reader* reader);

static Slice hmacImpl(Memory* mem, Slice key, Reader* reader, Size block_len, HashFunc hash_func) {
    ScratchMemory scratch = scratchMemory(mem);

    if (key.length > block_len) {
        key = hash_func(scratch.mem, readerFromSlice(scratch.mem, key));
    }
    U8 inner_key[block_len];
    U8 outer_key[block_len];
    for (Size i=0; i<block_len; i++) {
        inner_key[i] = i < key.length ? key.start[i] ^ 0x36 : 0x36;
        outer_key[i] = i < key.length ? key.start[i] ^ 0x5c : 0x5c;
    }
    Reader* inner_reader = readerNew(scratch.mem, reader->count + 1);
    readerAppend(inner_reader, inner_key, block_len);
    readerAppend(inner_reader, reader);
    Slice inner_hash = hash_func(scratch.mem, inner_reader);
    Reader* outer_reader = readerNew(scratch.mem, 2);
    readerAppend(outer_reader, outer_key, block_len);
    readerAppend(outer_reader, inner_hash);
    Slice outer_hash = hash_func(mem, outer_reader);

    scratchRelease(scratch);

    return outer_hash;
}

Slice hmacReader(Memory* mem, HashAlgorithm hash, Slice key, Reader* reader) {
    switch (hash) {
        case SHA_1:
            return hmacImpl(mem, key, reader, 64, sha_1_Reader);
        case SHA_2_224:
            return hmacImpl(mem, key, reader, 64, sha_2_224_Reader);
        case SHA_2_256:
            return hmacImpl(mem, key, reader, 64, sha_2_256_Reader);
        case SHA_2_384:
            return hmacImpl(mem, key, reader, 128, sha_2_384_Reader);
        case SHA_2_512:
            return hmacImpl(mem, key, reader, 128, sha_2_512_Reader);
    }
}

Slice hmacSlice(Memory* mem, HashAlgorithm hash, Slice key, Slice slice) {
    Reader* reader = readerFromSlice(mem, slice);
    switch (hash) {
        case SHA_1:
            return hmacImpl(mem, key, reader, 64, sha_1_Reader);
        case SHA_2_224:
            return hmacImpl(mem, key, reader, 64, sha_2_224_Reader);
        case SHA_2_256:
            return hmacImpl(mem, key, reader, 64, sha_2_256_Reader);
        case SHA_2_384:
            return hmacImpl(mem, key, reader, 128, sha_2_384_Reader);
        case SHA_2_512:
            return hmacImpl(mem, key, reader, 128, sha_2_512_Reader);
    }
}