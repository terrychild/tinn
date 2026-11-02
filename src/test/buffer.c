#include "test/expect.h"

#include "lib/macros.h"
#include "lib/types.h"
#include "lib/cli.h"
#include "lib/mem/memory.h"
#include "lib/mem/arena.h"
#include "lib/mem/pool.h"
#include "lib/mem/buffer.h"
#include "lib/bytes.h"

void testBuffer() {
    PRINT(CC_BLUE, "================\n Buffer tests\n================\n");
    ScratchMemory scratch = scratchMemory(NULL);
    Memory* mem = scratch.mem;

    Buffer* buf = bufNew(mem, 8);
    expect("length", buf->length, 0);
    expect("size", buf->size, 8);
    expect("arena allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 8);

    bufAppend(buf, (U8*)"hello world", 5);
    expect("Appeding data", bufAsStr(buf), "hello");
    expect(" length", buf->length, 5);
    expect(" size", buf->size, 8);
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 8);

    bufAppend(buf, " world!");
    expect("Appeding a string", bufAsStr(buf), "hello world!");
    expect(" length", buf->length, 12);
    expect(" size", buf->size, 16);
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 16);

    bufReset(buf);
    expect("reseting", bufAsStr(buf), "");
    expect(" length", buf->length, 0);
    expect(" size", buf->size, 16);
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 16);

    BufferSpace space = bufReadyWrite(buf, 12);
    expect("space length", space.length, 16);
    expect(" space start", space.start, buf->start);
    expect(" length", buf->length, 0);
    expect(" size", buf->size, 16);
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 16);

    for (int i=0; i<12; i++) {
        space.start[i] = 65+i;
    }
    bufConfirmWrite(buf, 12);
    expect("direct write", bufAsStr(buf), "ABCDEFGHIJKL");
    expect(" length", buf->length, 12);
    expect(" size", buf->size, 16);
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 16);

    space = bufReadyWrite(buf, 14);
    expect("space length", space.length, 20);
    expect(" length", buf->length, 12);
    expect(" size", buf->size, 32);
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 32);
    for (int i=0; i<14; i++) {
        space.start[i] = 65+12+i;
    }
    bufConfirmWrite(buf, 14);
    expect("direct write 2", bufAsStr(buf), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    expect(" length", buf->length, 26);
    expect(" size", buf->size, 32);
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 32);

    hexDump(bufAsSlice(buf));

    allocate(mem, 1);
    expect("allocate something else", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 32 + sizeof(Allocation) + 8);

    bufAppend(buf, "01234");
    expect("after next append", buf->length, 31);
    expect(" size", buf->size, 32);
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 32 + sizeof(Allocation) + 8);

    bufAppend(buf, "5");
    expect("after resize and move", bufAsStr(buf), "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345");
    expect(" length", buf->length, 32);
    expect(" size", buf->size, 64); // the null terminator required more space
    expect(" total allocated", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 32 + sizeof(Allocation) + 8 + sizeof(Allocation) + 64);

    memoryDebug(mem);
    memoryReset(mem);
    memoryDebug(mem);
    expect("allocator after reset", mem->arena->allocated, sizeof(Arena) + sizeof(Memory));

    allocate(mem, 48);
    buf = bufNew(mem, 8);
    bufAppend(buf, "more testing");
    expect("allocator with data and buf", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + 48 + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 16);
    memoryDebug(mem);
    buf = bufNew(mem, 8);
    bufAppend(buf, "double buffer time!");
    expect("allocator with data and two buffers", mem->arena->allocated, sizeof(Arena) + sizeof(Memory) + sizeof(Allocation) + 48 + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 16 + sizeof(Allocation) + sizeof(Buffer) + sizeof(Allocation) + 32);
    memoryDebug(mem);
    memoryReset(mem);

    scratchRelease(scratch);
}