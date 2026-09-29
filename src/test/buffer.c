#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/allocator.h"
#include "lib/mem/arena.h"
#include "lib/mem/pool.h"
#include "lib/mem/buffer.h"
#include "lib/bytes.h"

void testBuffer(Allocator* allocator) {
    PRINT(CC_BLUE, "================\n Buffer tests\n================\n");

    Buffer* buf = bufNew(allocator, 8);
    expect("length", buf->length, 0);
    expect("size", buf->size, 8);
    expect("arena allocated", buf->arena_allocated, buf->arena->allocated);
    expect("total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 8);

    bufAppend(buf, (U8*)"hello world", 5);
    expect("Appeding data", bufAsStr(buf), "hello");
    expect(" length", buf->length, 5);
    expect(" size", buf->size, 8);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 8);

    bufAppendStr(buf, " world!");
    expect("Appeding a string", bufAsStr(buf), "hello world!");
    expect(" length", buf->length, 12);
    expect(" size", buf->size, 16);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 16);

    bufReset(buf);
    expect("reseting", bufAsStr(buf), "");
    expect(" length", buf->length, 0);
    expect(" size", buf->size, 16);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 16);

    BufferSpace space = bufReadyWrite(buf, 12);
    expect("space length", space.length, 16);
    expect(" space start", space.start, buf->start);
    expect(" length", buf->length, 0);
    expect(" size", buf->size, 16);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 16);

    for (int i=0; i<12; i++) {
        space.start[i] = 65+i;
    }
    bufConfirmWrite(buf, 12);
    expect("direct write", bufAsStr(buf), "ABCDEFGHIJKL");
    expect(" length", buf->length, 12);
    expect(" size", buf->size, 16);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 16);

    space = bufReadyWrite(buf, 14);
    expect("space length", space.length, 20);
    expect(" length", buf->length, 12);
    expect(" size", buf->size, 32);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 32);
    for (int i=0; i<14; i++) {
        space.start[i] = 65+12+i;
    }
    bufConfirmWrite(buf, 14);
    expect("direct write 2", bufAsStr(buf), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    expect(" length", buf->length, 26);
    expect(" size", buf->size, 32);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 32);

    hexDump(bufAsSlice(buf));

    allocate(allocator, 1);
    expect("allocate something else", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 32 + 8);

    bufAppendStr(buf, "012345");
    expect("after next append", buf->length, 32);
    expect(" size", buf->size, 32);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 32 + 8);

    bufAppendStr(buf, "6");
    expect("after resize and move", bufAsStr(buf), "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456");
    expect(" length", buf->length, 33);
    expect(" size", buf->size, 64);
    expect(" total allocated", buf->arena->allocated, sizeof(Arena) + sizeof(Allocator) + sizeof(Buffer) + 32 + 8 + 64);

    allocatorDebug(allocator);
    allocatorReset(allocator);
    allocatorDebug(allocator);
    expect("allocator after pop", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator));

    allocate(allocator, 48);
    buf = bufNew(allocator, 8);
    bufAppendStr(buf, "more testing");
    expect("allocator with data and buf", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + 48 + sizeof(Buffer) + 16);
    allocatorDebug(allocator);
    buf = bufNew(allocator, 8);
    bufAppendStr(buf, "double buffer time!");
    expect("allocator with data and two buffers", allocator->arena->allocated, sizeof(Arena) + sizeof(Allocator) + 48 + sizeof(Buffer) + 16 + sizeof(Buffer) + 32);
    allocatorDebug(allocator);
    allocatorReset(allocator);
}