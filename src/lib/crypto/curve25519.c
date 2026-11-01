#include <stdlib.h>

#include "lib/crypto/curve25519.h"
#include "lib/crypto/mc.h"
#include "lib/mem/memory.h"
#include "lib/maths/bigint.h"

static MontgomeryCurve* curve = NULL;

BigInt* curve25519Key_b(Memory* mem, const BigInt* k) {
    if (curve == NULL) {
        curve = mcNew(programMemory(), "76d06", "1", "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed", "9");
    }
    return mcKey(mem, curve, k);
}
BigInt* curve25519Key_h(Memory* mem, const char* hex) {
    ScratchMemory scratch = scratchMemory(mem);
    BigInt* x = biClone(mem, curve25519Key_b(scratch.mem, biFromHex(scratch.mem, hex)));
    scratchRelease(scratch);
    return x;
}
BigInt* curve25519Key_i(Memory* mem, U64 i) {
    ScratchMemory scratch = scratchMemory(mem);
    BigInt* x = biClone(mem, curve25519Key_b(scratch.mem, biFromInt(scratch.mem, i)));
    scratchRelease(scratch);
    return x;
}