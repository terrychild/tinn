#ifndef LIB_MEM_MFILE_H
#define LIB_MEM_MFILE_H

#include <time.h>

#include "lib/types.h"
#include "lib/sys/polling.h"

struct MappedFile {
    Polling* polling;
    const char* path;
    int wd;
    time_t mod_date;
    U64 size;
    U64 length;
    U8* start;
};

bool mfileOpen(MappedFile* file, Polling* polling, const char* path);
void mfileClose(MappedFile* file);

Slice mfileAsSlice(MappedFile* file);

#endif