#ifndef LIB_CRYPTO_AES_H
#define LIB_CRYPTO_AES_H

#include "lib/types.h"

Slice aes_gcs_encrypt(Memory* mem, Slice key, Slice iv, Slice plain_text, Slice additional_data);

#endif