#ifndef LIB_MEM_ALIGN_H
#define LIB_MEM_ALIGN_H

#include <lib/types.h>

Size align(Size ptr, Size multiple);
Size alignToPage(Size ptr);
Size alignToWord(Size ptr);

#endif