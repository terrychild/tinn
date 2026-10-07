#ifndef LIB_MEM_MFILE_H
#define LIB_MEM_MFILE_H

#include <time.h>

#include "lib/types.h"

struct MappedFile {
    const char* path;
    U64 size;
    U64 length;
    U8* start;
};

bool mfileOpen(MappedFile* file, const char* path);
void mfileClose(MappedFile* file);

time_t mfileModDate(MappedFile* file);

Slice mfileAsSlice(MappedFile* file);

#endif