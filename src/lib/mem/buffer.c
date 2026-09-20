#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib/mem/buffer.h"
#include "lib/cli.h"

Buffer* bufNew(Arena* arena, U64 initial_size, U64 max_size) {
    Buffer* buf = arenaAlloc(arena, sizeof(Buffer));
    bufInit(buf, arena, initial_size, max_size);
    return buf;
}
void bufInit(Buffer* buf, Arena* arena, U64 initial_size, U64 max_size) {
    assert(initial_size > 0);
    buf->arena = arenaAddChild(arena, max_size, false);
    assert(buf->arena->size >= initial_size);

    buf->size = initial_size;
    buf->length = 0;
    buf->start = arenaAlloc(buf->arena, initial_size);
}
void bufReset(Buffer* buf) {
    buf->length = 0;
}

static void ensure(Buffer* buf, U64 n) {
    U64 new_size = buf->size;
    while (new_size < buf->length + n) {
        new_size *= 2;
    }
    if (new_size > buf->size) {
        arenaAlloc(buf->arena, new_size - buf->size);
        buf->size = new_size;
    }
}

void bufAppend(Buffer* buf, const U8* data, U64 n) {
    ensure(buf, n);

    memcpy(buf->start + buf->length, data, n);
    buf->length += n;
}
void bufAppendStr(Buffer* buf, const char* str) {
    bufAppend(buf, (U8*)str, strlen(str));  
}

BufferSpace bufReadyWrite(Buffer* buf, U64 min_size) {
    ensure(buf, min_size);
    return (BufferSpace) {
        .length = buf->size - buf->length,
        .start = buf->start + buf->length
    };
}
void bufConfirmWrite(Buffer* buf, U64 n) {
    assert(buf->length + n <= buf->size);
    buf->length += n;
}

char* bufAsStr(Buffer* buf) {
    ensure(buf, 1);
    buf->start[buf->length] = '\0';
    return (char*)buf->start;
}

void bufHexDump(Buffer* buf) {
    bool skipped = false;
    for (U64 i=0; i < buf->size; i+=16) {
        bool has_data = false;
        for (U64 j=0; j<16 && !has_data; j++) {
            if (i+j < buf->size && buf->start[i+j] > 0) {
                has_data = true;
            }
        }

        if (!has_data) {
            skipped = true;
        } else {
            PRINT(skipped ? CC_BOLD_WHITE : CC_WHITE, "%08lX  ", i);
            skipped = false;
            for (U64 j=0; j<16; j++) {
                if (i+j < buf->size) {
                    PRINT(i+j < buf->length ? CC_YELLOW : CC_BRIGHT_BLACK, "%02X ", buf->start[i+j]);
                } else {
                    PRINT(CC_NULL, "   ");
                }
                if (j==7) {
                    PRINT(CC_NULL, " ");
                }
            }
            PRINT(CC_NULL, " ");

            for (U64 j=0; j<16; j++) {
                if (i+j < buf->size && buf->start[i+j] > 32 && buf->start[i+j] < 127) {
                    PRINT(i+j < buf->length ? CC_CYAN : CC_BRIGHT_BLACK, "%c", buf->start[i+j]);
                } else {
                    PRINT(CC_NULL, " ");
                }
            }
            PRINT(CC_NULL, "\n");
        }
    }
}