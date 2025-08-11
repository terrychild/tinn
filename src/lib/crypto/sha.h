#ifndef LIB_CRYPTO_SHA_H
#define LIB_CRYPTO_SHA_H

#include "lib/types.h"

bool sha1(U8 dest[20], U8* source, U64 len);

//int sha_cmd(int argc, char* argv[]);

#endif