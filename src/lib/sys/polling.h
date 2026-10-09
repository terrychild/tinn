#ifndef LIB_SYS_POLLING_H
#define LIB_SYS_POLLING_H

#include <poll.h>

#include "lib/types.h"

typedef void (*PollingCallbackFunc)(struct pollfd* pfd, void* context, bool* remove);
typedef void (*PollingFileCallbackFunc)(void* context);

typedef struct {
    PollingCallbackFunc func;
    void* context;
} PollingCallback;

typedef struct {
    int wd;
    PollingFileCallbackFunc func;
    void* context;
} PollingFileCallback;

typedef struct {
    Array* pollfds;
    Array* callbacks;
    int inotify;
    Pool* files;
} Polling;

Polling* pollingNew(Allocator* allocator, U64 max_capacity);
void pollingInit(Polling* polling, Allocator* allocator, U64 max_capacity);

void pollingAdd(Polling* polling, int fd, PollingCallback callback);
void pollingRemove(Polling* polling, int fd);
void pollingEvents(Polling* polling, int fd, short events);

void pollingPoll(Polling* polling);

int pollingMonitorFile(Polling* polling, const char* path, PollingFileCallback callback);
void pollingUnmonitorFile(Polling* polling, int wd);

#endif