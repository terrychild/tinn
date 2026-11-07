#include <stdlib.h>

#include "lib/crypto/curve25519.h"
#include "lib/crypto/mc.h"
#include "lib/mem/memory.h"
#include "lib/maths/bigint.h"

#include "lib/bytes.h"

static MontgomeryCurve* curve = NULL;
static BigInt* key_mod = NULL;

static void initCurve() {
    curve = mcNew(programMemory(), "0x76d06", "1", "0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed", "9");
    key_mod = biPow(programMemory(), 2, 255);
}
static BigInt* clampK(BigInt* k) {
    biMod(k, key_mod);
    k->limbs[0] &= 0xFFFFFFFFFFFFFFF8;
    k->limbs[3] &= 0x7FFFFFFFFFFFFFFF;
    k->limbs[3] |= 0x4000000000000000;
    hexDump(biToBESlice(k->mem, k));
    return k;
}

BigInt* curve25519Key_b(Memory* mem, const BigInt* k) {
    if (curve == NULL) {
        initCurve();
    }
    ScratchMemory scratch = scratchMemory(mem);
    BigInt* clean_k = clampK(biFrom(scratch.mem, k));
    BigInt* x = mcKey(mem, curve, clean_k);
    scratchRelease(scratch);
    return x;
}
BigInt* curve25519Key_s(Memory* mem, const char* str) {
    ScratchMemory scratch = scratchMemory(mem);
    BigInt* x = biFrom(mem, curve25519Key_b(scratch.mem, biFrom(scratch.mem, str)));
    scratchRelease(scratch);
    return x;
}
BigInt* curve25519Key_i(Memory* mem, U64 i) {
    ScratchMemory scratch = scratchMemory(mem);
    BigInt* x = biFrom(mem, curve25519Key_b(scratch.mem, biFromInt(scratch.mem, i)));
    scratchRelease(scratch);
    return x;
}
BigInt* curve25519Shared(Memory* mem, const BigInt* point, const BigInt* k) {
    if (curve == NULL) {
        initCurve();
    }
    ScratchMemory scratch = scratchMemory(mem);
    BigInt* clean_k = clampK(biFrom(scratch.mem, k));
    BigInt* clean_point = biMod(scratch.mem, point, curve->p);
    BigInt* x = biFrom(mem, mcShared(scratch.mem, curve, clean_point, clean_k));
    scratchRelease(scratch);
    return x;
}