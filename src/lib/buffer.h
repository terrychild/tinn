#ifndef LIB_BUFFER_H
#define LIB_BUFFER_H

#include <stdint.h>

typedef struct {
    long size;
    long length;
    uint8_t* data;
} Buffer;

/*Buffer* buf_new(long size);
void buf_free(Buffer* buf);
*/
#endif