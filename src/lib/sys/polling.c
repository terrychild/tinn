#include <unistd.h>

#include "lib/sys/polling.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/log.h"

Polling* pollingNew(Allocator* allocator, U64 max_capacity) {
    Polling* polling = allocate(allocator, sizeof(*polling));
    pollingInit(polling, allocator, max_capacity);
    return polling;
}
void pollingInit(Polling* list, Allocator* allocator, U64 max_capacity) {
    U64 size = 8;
    max_capacity = max_capacity ? max_capacity : 256;

    list->pollfds = arrayNew(allocator, sizeof(struct pollfd), size, max_capacity);
    list->callbacks = arrayNew(allocator, sizeof(PollingCallback), size, max_capacity);
}

void pollingAdd(Polling* list, int fd, PollingCallback callback) {
    arrayPush(list->pollfds, &(struct pollfd){
        .fd = fd,
        .events = POLLIN,
        .revents = 0
    });
    arrayPush(list->callbacks, &callback);
}
void pollingRemove(Polling* list, int fd) {
    for (U64 i = 0; i < list->pollfds->count; i++) {
        struct pollfd* pfd = (struct pollfd*)arrayGet(list->pollfds, i);
        if (pfd->fd == fd) {
            close(pfd->fd);
            arraySet(list->pollfds, i, arrayPop(list->pollfds));
            arraySet(list->callbacks, i, arrayPop(list->callbacks));
            return;
        }
    }
}
void pollingEvents(Polling* list, int fd, short events) {
    for (U64 i = 0; i < list->pollfds->count; i++) {
        struct pollfd* pfd = (struct pollfd*)arrayGet(list->pollfds, i);
        if (pfd->fd == fd) {
            pfd->events = events;
            return;
        }
    }
}

void pollingPoll(Polling* list) {
    while (list->pollfds->count > 0) {
        if (poll((struct pollfd*)list->pollfds->start, list->pollfds->count, -1) < 0 ) {
            PANIC("When polling");
        }

        for (U64 i = 0; i < list->pollfds->count; i++) {
            struct pollfd* pfd = (struct pollfd*)arrayGet(list->pollfds, i);
            if (pfd->revents) {
                bool and_close = false;

                PollingCallback* callback = (PollingCallback*)arrayGet(list->callbacks, i);

                callback->func(pfd, callback->context, &and_close);

                if (and_close) {
                    close(pfd->fd);
                    arraySet(list->pollfds, i, arrayPop(list->pollfds));
                    arraySet(list->callbacks, i, arrayPop(list->callbacks));
                    i--;
                }
            }
        }
    }
}