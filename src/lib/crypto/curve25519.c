#include <stdlib.h>

#include "lib/crypto/curve25519.h"
#include "lib/crypto/mc.h"
#include "lib/maths/bigint.h"

BigInt* curve25519Key(Allocator* allocator, BigInt* k) {
    MontgomeryCurve* curve = mcNew(allocator, "76d06", "1", "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed", "9");
    BigInt* x = biNew(allocator);
    mcKey(x, curve, k);
    return x;
}