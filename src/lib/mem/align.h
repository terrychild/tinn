#ifndef LIB_MEM_ALIGN_H
#define LIB_MEM_ALIGN_H

#include <lib/types.h>

U64 align(U64 ptr, U64 multiple);
U64 alignToPage(U64 ptr);
U64 alignToWord(U64 ptr);

#endif