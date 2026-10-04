#ifndef LIB_MEM_ALLOCATOR_H
#define LIB_MEM_ALLOCATOR_H

#include "lib/types.h"

struct Allocator {
    Arena* arena;
    Pool* children;
    Pool* files;
};

Allocator* allocatorNew(U64 size);
void allocatorInit(Allocator* allocator, Arena* arena);
void allocatorReset(Allocator* allocator);
void allocatorRelease(Allocator* allocator);

Allocator* allocateChild(Allocator* allocator, U64 size);
void deallocateChild(Allocator* allocator, Allocator* child);

MappedFile* allocateFile(Allocator* allocator, const char* path);
void deallocateFile(Allocator* allocator, MappedFile* child);

void* allocate(Allocator* allocator, U64 size);

void allocatorDebug(Allocator* allocator);

#endif