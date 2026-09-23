#include <unistd.h>

#include "lib/net/sockets.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/log.h"

Sockets* socketsNew(Allocator* allocator, U64 max_capacity) {
    Sockets* sockets = allocate(allocator, sizeof(*sockets));
    socketsInit(sockets, allocator, max_capacity);
    return sockets;
}
void socketsInit(Sockets* list, Allocator* allocator, U64 max_capacity) {
    U64 size = 8;
    max_capacity = max_capacity ? max_capacity : 256;

    list->pollfds = arrayNew(allocator, sizeof(struct pollfd), size, max_capacity);
    list->callbacks = arrayNew(allocator, sizeof(SocketCallback), size, max_capacity);
}

void socketsAdd(Sockets* list, int new_socket, SocketCallback callback) {
    arrayPush(list->pollfds, &(struct pollfd){
        .fd = new_socket,
        .events = POLLIN,
        .revents = 0
    });
    arrayPush(list->callbacks, &callback);
}
void socketsRemove(Sockets* list, int old_socket) {
    for (U64 i = 0; i < list->pollfds->count; i++) {
        struct pollfd* pfd = (struct pollfd*)arrayGet(list->pollfds, i);
        if (pfd->fd == old_socket) {
            close(pfd->fd);
            arraySet(list->pollfds, i, arrayPop(list->pollfds));
            arraySet(list->callbacks, i, arrayPop(list->callbacks));
            return;
        }
    }
}

void socketsPoll(Sockets* list) {
    while (list->pollfds->count > 0) {
        if (poll((struct pollfd*)list->pollfds->start, list->pollfds->count, -1) < 0 ) {
            PANIC("When polling");
        }

        for (U64 i = 0; i < list->pollfds->count; i++) {
            struct pollfd* pfd = (struct pollfd*)arrayGet(list->pollfds, i);
            if (pfd->revents) {
                bool close_socket = false;

                SocketCallback* callback = (SocketCallback*)arrayGet(list->callbacks, i);

                callback->eventFunc(pfd, callback->context, &close_socket);

                if (close_socket) {
                    if (callback->closeFunc) {
                        callback->closeFunc(callback->context);
                    }
                    close(pfd->fd);

                    arraySet(list->pollfds, i, arrayPop(list->pollfds));
                    arraySet(list->callbacks, i, arrayPop(list->callbacks));
                    i--;
                }
            }
        }
    }
}