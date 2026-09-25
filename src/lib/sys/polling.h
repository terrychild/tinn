#ifndef LIB_SYS_POLLING_H
#define LIB_SYS_POLLING_H

#include <poll.h>

#include "lib/types.h"

typedef void (*PollingCallbackFunc)(struct pollfd* pfd, void* context, bool* close);

typedef struct {
    PollingCallbackFunc func;
    void* context;
} PollingCallback;

typedef struct {
    Array* pollfds;
    Array* callbacks;
} Polling;

Polling* pollingNew(Allocator* allocator, U64 max_capacity);
void pollingInit(Polling* list, Allocator* allocator, U64 max_capacity);

void pollingAdd(Polling* list, int fd, PollingCallback callback);
void pollingRemove(Polling* list, int fd);
void pollingEvents(Polling* list, int fd, short events);

void pollingPoll(Polling* list);

#endif