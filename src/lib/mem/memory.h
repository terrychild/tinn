#ifndef LIB_MEM_MEMORY_H
#define LIB_MEM_MEMORY_H

#include "lib/types.h"
#include "lib/sys/polling.h"

typedef struct {
    U64 size;
    U8 data[];
} Allocation;

struct Memory {
    Arena* arena;
    Pool* children;
    Pool* files;
    Allocation* last_allocation;
};

Memory* memoryNew(U64 size);
void memoryInit(Memory* mem, Arena* arena);
void memoryReset(Memory* mem);
void memoryRelease(Memory* mem);

Memory* allocateChild(Memory* mem, U64 size);
void deallocateChild(Memory* mem, Memory* child);

MappedFile* allocateFile(Memory* mem, Polling* polling, const char* path);
void deallocateFile(Memory* mem, MappedFile* child);

void* allocate(Memory* mem, U64 size);
void* reallocate(Memory* mem, void* original, U64 size);

void memoryDebug(Memory* mem);

#endif