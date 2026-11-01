#include <stdlib.h>

#include "lib/crypto/curve25519.h"
#include "lib/crypto/mc.h"
#include "lib/maths/bigint.h"

BigInt* curve25519Key(Memory* mem, BigInt* k) {
    MontgomeryCurve* curve = mcNew(mem, "76d06", "1", "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed", "9");
    BigInt* x = biNew(mem);
    mcKey(x, curve, k);
    return x;
}