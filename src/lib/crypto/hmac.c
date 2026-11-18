#include "lib/crypto/hmac.h"
#include "lib/crypto/sha.h"
#include "lib/mem/memory.h"
#include "lib/mem/reader.h"

Slice hmac(Memory* mem, Slice key, Reader* reader, Size block_len, Hash hash_func) {
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

Slice hmacSha224Slices(Memory* mem, const Slice key, Reader* reader) {
    return hmac(mem, key, reader, 64, sha224Reader);
}
Slice hmacSha224Slice(Memory* mem, const Slice key, const Slice source) {
    ScratchMemory scratch = scratchMemory(mem);
    Slice hash = hmac(mem, key, readerFromSlice(scratch.mem, source), 64, sha224Reader);
    scratchRelease(scratch);
    return hash;
}

Slice hmacSha256Slices(Memory* mem, const Slice key, Reader* reader) {
    return hmac(mem, key, reader, 64, sha256Reader);
}
Slice hmacSha256Slice(Memory* mem, const Slice key, const Slice source) {
    ScratchMemory scratch = scratchMemory(mem);
    Slice hash = hmac(mem, key, readerFromSlice(scratch.mem, source), 64, sha256Reader);
    scratchRelease(scratch);
    return hash;
}

Slice hmacSha384Slices(Memory* mem, const Slice key, Reader* reader) {
    return hmac(mem, key, reader, 128, sha384Reader);
}
Slice hmacSha384Slice(Memory* mem, const Slice key, const Slice source) {
    ScratchMemory scratch = scratchMemory(mem);
    Slice hash = hmac(mem, key, readerFromSlice(scratch.mem, source), 128, sha384Reader);
    scratchRelease(scratch);
    return hash;
}

Slice hmacSha512Slices(Memory* mem, const Slice key, Reader* reader) {
    return hmac(mem, key, reader, 128, sha512Reader);
}
Slice hmacSha512Slice(Memory* mem, const Slice key, const Slice source) {
    ScratchMemory scratch = scratchMemory(mem);
    Slice hash = hmac(mem, key, readerFromSlice(scratch.mem, source), 128, sha512Reader);
    scratchRelease(scratch);
    return hash;
}