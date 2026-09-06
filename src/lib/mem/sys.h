#ifndef LIB_MEM_SYS_H
#define LIB_MEM_SYS_H

#include "lib/types.h"

// align pointers/lengths to page/word boundaries
U64 alignToPage(U64 ptr);
U64 alignToWord(U64 ptr);

// system calls
void* sysMemReserve(U64 size);
void sysMemCommit(void* memory, U64 size);
void sysMemUncommit(void* memory, U64 size);
void sysMemRelease(void* memory, U64 size);

#endif