#include <unistd.h>
#include <assert.h>
#include <sys/inotify.h>

#include "lib/sys/polling.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/mem/pool.h"
#include "lib/log.h"
#include "lib/macros.h"

Polling* pollingNew(Allocator* allocator, U64 capacity) {
    Polling* polling = allocate(allocator, sizeof(*polling));
    pollingInit(polling, allocator, capacity);
    return polling;
}
void pollingInit(Polling* polling, Allocator* allocator, U64 capacity) {
    assert(capacity > 0);

    polling->pollfds = arrayNew(allocator, sizeof(struct pollfd), capacity);
    polling->callbacks = arrayNew(allocator, sizeof(PollingCallback), capacity);
    polling->inotify = -1;
    polling->files = poolNew(allocator, sizeof(PollingFileCallback), capacity);
}

void pollingAdd(Polling* polling, int fd, PollingCallback callback) {
    arrayPush(polling->pollfds, &(struct pollfd){
        .fd = fd,
        .events = POLLIN,
        .revents = 0
    });
    arrayPush(polling->callbacks, &callback);
}
void pollingRemove(Polling* polling, int fd) {
    for (U64 i = 0; i < polling->pollfds->count; i++) {
        struct pollfd* pfd = (struct pollfd*)arrayGet(polling->pollfds, i);
        if (pfd->fd == fd) {
            arraySet(polling->pollfds, i, arrayPop(polling->pollfds));
            arraySet(polling->callbacks, i, arrayPop(polling->callbacks));
            return;
        }
    }
}
void pollingEvents(Polling* polling, int fd, short events) {
    for (U64 i = 0; i < polling->pollfds->count; i++) {
        struct pollfd* pfd = (struct pollfd*)arrayGet(polling->pollfds, i);
        if (pfd->fd == fd) {
            pfd->events = events;
            return;
        }
    }
}

void pollingPoll(Polling* polling) {
    while (polling->pollfds->count > 0) {
        if (poll((struct pollfd*)polling->pollfds->start, polling->pollfds->count, -1) < 0 ) {
            PANIC("When polling");
        }

        for (U64 i = 0; i < polling->pollfds->count; i++) {
            struct pollfd* pfd = (struct pollfd*)arrayGet(polling->pollfds, i);
            if (pfd->revents) {
                PollingCallback* callback = (PollingCallback*)arrayGet(polling->callbacks, i);
                callback->func(pfd, callback->context);

                if (pfd->events == 0) {
                    arraySet(polling->pollfds, i, arrayPop(polling->pollfds));
                    arraySet(polling->callbacks, i, arrayPop(polling->callbacks));
                    i--;
                }
            }
        }
    }
}

static void monitorFile(struct pollfd* pfd, void* context){
    Polling* polling = context;
    // TODO check for POLL errors?
    if (pfd->revents & POLLIN) {
        char buffer[KB(4)];
        char* ptr;
        int read_len, event_len;

        while ((read_len = read(polling->inotify, buffer, sizeof(buffer))) > 0) {
            for (ptr = buffer; ptr < buffer + read_len; ptr += event_len) {
                struct inotify_event* event = (struct inotify_event*)ptr;
                event_len = sizeof(struct inotify_event) + event->len;
                if (event->mask & IN_MODIFY) {
                    PoolNode* node = polling->files->first;
                    while (node != NULL) {
                        PollingFileCallback* callback = (PollingFileCallback*)poolData(node);
                        if (callback->wd == event->wd) {
                            callback->func(callback->context);
                            break;
                        }
                        node = node->next;
                    }
                }
            }
        }
    }
}
int pollingMonitorFile(Polling* polling, const char* path, PollingFileCallback callback) {
    if (polling->inotify < 0) {
        polling->inotify = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
        if (polling->inotify < 0) {
            ERROR("Unable to initalise inotify");
            return -1;
        }

        pollingAdd(polling, polling->inotify, (PollingCallback) {
            .func = monitorFile,
            .context = polling
        });
    }

    callback.wd = inotify_add_watch(polling->inotify, path, IN_MODIFY);
    if (callback.wd < 0) {
        ERROR("Unable to monitor file \"%s\"", path);
        return -1;
    }

    poolPush(polling->files, &callback);

    return callback.wd;
}

//TODO this function steps thought the pool twice, bad, must be a better way
void pollingUnmonitorFile(Polling* polling, int wd) {
    PoolNode* node = polling->files->first;
    while (node != NULL) {
        PollingFileCallback* callback = (PollingFileCallback*)poolData(node);
        if (callback->wd == wd) {
            poolRemove(polling->files, callback);
            return;
        }
        node = node->next;
    }
}