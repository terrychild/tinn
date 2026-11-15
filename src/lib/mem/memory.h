#ifndef LIB_MEM_MEMORY_H
#define LIB_MEM_MEMORY_H

#include "lib/types.h"
#include "lib/sys/polling.h"

typedef struct {
    Size size;
    U8 data[];
} Allocation;

struct Memory {
    Arena* arena;
    Pool* children;
    Pool* files;
    Allocation* last_allocation;
};

typedef struct {
    Memory* mem;
    Size allocated;
} ScratchMemory;

Memory* memoryNew(Size size);
void memoryInit(Memory* mem, Arena* arena);
void memoryReset(Memory* mem);
void memoryRelease(Memory* mem);

Memory* allocateChild(Memory* mem, Size size);
void deallocateChild(Memory* mem, Memory* child);

MappedFile* allocateFile(Memory* mem, Polling* polling, const char* path);
void deallocateFile(Memory* mem, MappedFile* child);

void* allocate(Memory* mem, Size size);
void* reallocate(Memory* mem, void* original, Size size);

Memory* programMemory();
ScratchMemory scratchMemory(Memory* current);
void scratchRelease(ScratchMemory scratch);

void memoryDebug(Memory* mem);

#endif