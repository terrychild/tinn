#ifndef LIB_MEM_MFILE_H
#define LIB_MEM_MFILE_H

#include "lib/types.h"

struct MappedFile {
    U64 size;
    U64 length;
    U8* start;
};

bool mfileOpen(MappedFile* file, const char* path);
void mfileClose(MappedFile* file);

Slice mfileAsSlice(MappedFile* file);

#endif