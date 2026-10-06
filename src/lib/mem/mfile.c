//#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lib/mem/mfile.h"
#include "lib/mem/align.h"
#include "lib/mem/slice.h"
#include "lib/log.h"

// file
bool mfileOpen(MappedFile* file, const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        ERROR("Unable to open file \"%s\"", path);
        return false;
    }

    struct stat stats;
    if (fstat(fd, &stats) == -1) {
        close(fd);
        ERROR("fstat failed on file \"%s\"", path);
        return false;
    }

    file->length = stats.st_size;
    file->size = alignToPage(file->length);
    file->start = mmap(NULL, file->size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file->start == MAP_FAILED) {
        close(fd);
        ERROR("Unable to map memory for file \"%s\"", path);
        return false;
    }

    close(fd);

    return true;
}

void mfileClose(MappedFile* file) {
    if (file) {
        if (munmap(file->start, file->size)) {
            ERROR("Unable to free memory");
        }
    }
}

Slice mfileAsSlice(MappedFile* file) {
    if (file) {
        return (Slice) {
            .length = file->length,
            .start = file->start
        };
    } else {
        return sliceEmpty();
    }
}