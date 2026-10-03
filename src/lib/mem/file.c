//#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lib/mem/file.h"
#include "lib/mem/align.h"
#include "lib/mem/slice.h"
#include "lib/log.h"

// file
File fileRead(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        ERROR("Unable to open file \"%s\"", path);
        return (File) {0};
    }

    struct stat stats;
    if (fstat(fd, &stats) == -1) {
        close(fd);
        ERROR("fstat failed on file \"%s\"", path);
        return (File) {0};
    }

    U64 length = stats.st_size;
    U64 size = alignToPage(length);

    U8* mem = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mem == MAP_FAILED) {
        close(fd);
        ERROR("Unable to map memory for file \"%s\"", path);
        return (File) {0};
    }

    close(fd);

    return (File) {
        .size = size,
        .length = length,
        .start = mem
    };
}

void fileClose(File file) {
    if (munmap(file.start, file.size)) {
        ERROR("Unable to free memory");
    }
}

Slice fileAsSlice(File file) {
    return (Slice) {
        .length = file.length,
        .start = file.start
    };
}