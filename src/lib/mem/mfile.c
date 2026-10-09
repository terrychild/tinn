//#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lib/mem/mfile.h"
#include "lib/mem/align.h"
#include "lib/mem/slice.h"
#include "lib/log.h"

static bool mapFile(MappedFile* file, bool update) {
    int fd = open(file->path, O_RDONLY);
    if (fd == -1) {
        ERROR("Unable to open file \"%s\"", file->path);
        return false;
    }

    struct stat stats;
    if (fstat(fd, &stats) != 0) {
        ERROR("fstat failed on file \"%s\"", file->path);
        close(fd);
        return false;
    }

    if (update) {
        if (file->mod_date == stats.st_mtime && file->length == stats.st_size) {
            DEBUG("no update require");
            close(fd);
            return true;
        }
        if (file->start) {
            if (munmap(file->start, file->size)) {
                ERROR("Unable to free memory");
                close(fd);
                return false;
            }
        }
    }

    file->mod_date = stats.st_mtime;
    file->length = stats.st_size;
    file->size = alignToPage(file->length);

    if (file->size == 0) {
        //WARN("File size is 0 for file \"%s\"", file->path);
        file->start = NULL;
        close(fd);
        return false;
    }

    file->start = mmap(NULL, file->size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file->start == MAP_FAILED) {
        ERROR("Unable to map memory for file \"%s\"", file->path);
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

static void updateFile(void* context){
    MappedFile* file = context;
    mapFile(file, true);
}

bool mfileOpen(MappedFile* file, Polling* polling, const char* path) {
    file->polling = polling;
    file->path = path;

    if (!mapFile(file, false)) {
        return false;
    }

    file->wd = pollingMonitorFile(polling, path, (PollingFileCallback) {
        .func = updateFile,
        .context = file
    });
    if (file->wd < 0) {
        return false;
    }

    return true;
}

void mfileClose(MappedFile* file) {
    if (file) {
        pollingUnmonitorFile(file->polling, file->wd);
        if (file->start) {
            if (munmap(file->start, file->size)) {
                ERROR("Unable to free memory");
            }
        }
    }
}

Slice mfileAsSlice(MappedFile* file) {
    if (file && file->start) {
        return (Slice) {
            .length = file->length,
            .start = file->start
        };
    } else {
        return sliceEmpty();
    }
}