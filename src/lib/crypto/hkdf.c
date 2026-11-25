#include "lib/crypto/hkdf.h"
#include "lib/crypto/hmac.h"
#include "lib/mem/memory.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"

Slice hkdfExtract(Memory* mem, HashAlgorithm hash, Slice salt, Slice ikm) {
    return hmac(mem, hash, salt, ikm);
}

/*Slice hkdfExpandReader(Memory* mem, HashAlgorithm hash, Slice prk, Reader* info, Size length) {
    ScratchMemory scratch = scratchMemory(mem);

    Buffer* okm = bufNew(mem, length);
    Slice temp = sliceEmpty();
    while (okm->length < length) {
        Reader* data = readerNew(scratch.mem, info->count + 2);
        readerAppend(data, temp);
        readerAppend(data, info);

    }

    scratchRelease(scratch);
    return slice(buf, 0, length);
}*/
