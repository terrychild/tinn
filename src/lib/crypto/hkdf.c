#include "lib/crypto/hkdf.h"
#include "lib/crypto/hmac.h"
#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"

Slice hkdfExtract(Memory* mem, HashAlgorithm hash, Slice salt, Slice ikm) {
    return hmac(mem, hash, salt, ikm);
}

Slice hkdfExpand(Memory* mem, HashAlgorithm hash, Slice prk, Slice info, Size length) {
    ScratchMemory scratch = scratchMemory(mem);

    Buffer* okm = bufNew(mem, length);
    Buffer* message = bufNew(scratch.mem, hash_length[hash] + info.length + 1);
    Slice temp = sliceEmpty();
    for (U8 i=0; okm->length < length && i<255; i++) {
        bufReset(message);
        bufAppend(message, temp);
        bufAppend(message, info);
        bufAppendBE8(message, i+1);

        temp = hmac(mem, hash, prk, bufAsSlice(message));
        bufAppend(okm, temp);
    }

    scratchRelease(scratch);
    return slice(okm, 0, length);
}
