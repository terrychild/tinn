#include "buffer.h"

#include "mem.h"

/*Buffer* buf_new(long size) {
    Buffer* buf = allocate(NULL, sizeof(*buf));
    buf->size = size;
    buf->length = 0;
    buf->data = allocate(NULL, buf->size);
    return buf;
}
void buf_free(Buffer* buf) {
    if (buf != NULL) {
        free(buf->data);
        free(buf);
    }
}*/