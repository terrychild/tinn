#include <stdlib.h>

#include "lib/crypto/x25519.h"
#include "lib/crypto/mc.h"
#include "lib/mem/memory.h"
#include "lib/maths/bigint.h"

#include "lib/bytes.h"

static MontgomeryCurve* curve = NULL;
static BigInt* key_mod = NULL;
static BigInt* key_clamp1 = NULL;
static BigInt* key_clamp2 = NULL;

static void initCurve() {
    curve = mcNew(programMemory(), "0x76d06", "1", "0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed", "9");
    key_mod = biPow(programMemory(), 2, 255);
    key_clamp1 = biFrom(programMemory(), "0x7ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff8");
    key_clamp2 = biFrom(programMemory(), "0x4000000000000000000000000000000000000000000000000000000000000000");
}
static BigInt* clamp(BigInt* key) {
    biMod(key, key_mod);
    biAnd(key, key_clamp1);
    biOr(key, key_clamp2);
    return key;
}

BigInt* x25519PublicKey(Memory* mem, const BigInt* private_key) {
    if (curve == NULL) {
        initCurve();
    }
    ScratchMemory scratch = scratchMemory(mem);
    BigInt* x = mcPublicKey(mem, curve, clamp(biFrom(scratch.mem, private_key)));
    scratchRelease(scratch);
    return x;
}
BigInt* x25519(Memory* mem, const BigInt* scaler, const BigInt* point) {
    if (curve == NULL) {
        initCurve();
    }
    ScratchMemory scratch = scratchMemory(mem);
    BigInt* x = biFrom(mem, mc(scratch.mem, curve, clamp(biFrom(scratch.mem, scaler)), point));
    scratchRelease(scratch);
    return x;
}