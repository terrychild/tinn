#ifndef LIB_MEM_MFILE_H
#define LIB_MEM_MFILE_H

#include "lib/types.h"

struct File {
    U64 size;
    U64 length;
    U8* start;
};

File fileRead(const char* path);
void fileClose(File file);

Slice fileAsSlice(File file);

#endif