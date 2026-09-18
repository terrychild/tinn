#include <stdlib.h>
#include <unistd.h>

#include "lib/net/sockets.h"
#include "lib/console.h"

void socketsInit(Sockets* list, ArenaPool* arena_pool, U64 max_capacity) {
    U64 size = 8;
    max_capacity = max_capacity ? max_capacity : 256;

    arrayInit(&list->pollfds, arena_pool, sizeof(struct pollfd), size, max_capacity);
    arrayInit(&list->callbacks, arena_pool, sizeof(SocketCallback), size, max_capacity);
}

void socketsRelease(Sockets* list) {
    arrayRelease(&list->pollfds);
    arrayRelease(&list->callbacks);
}

void socketsAdd(Sockets* list, int new_socket, SocketCallback callback) {
    arrayPush(&list->pollfds, &(struct pollfd){
        .fd = new_socket,
        .events = POLLIN,
        .revents = 0
    });
    arrayPush(&list->callbacks, &callback);
}

void socketsRemove(Sockets* list, int old_socket) {
    for (U64 i = 0; i < list->pollfds.count; i++) {
        struct pollfd* pfd = (struct pollfd*)arrayGet(&list->pollfds, i);
        if (pfd->fd == old_socket) {
            close(pfd->fd);
            arraySet(&list->pollfds, i, arrayPop(&list->pollfds));
            arraySet(&list->callbacks, i, arrayPop(&list->callbacks));
            return;
        }
    }
}

void socketsPoll(Sockets* list) {
    while (list->pollfds.count > 0) {
        if (poll((struct pollfd*)list->pollfds.data, list->pollfds.count, -1) < 0 ) {
            PANIC("When polling");
        }

        for (U64 i = 0; i < list->pollfds.count; i++) {
            struct pollfd* pfd = (struct pollfd*)arrayGet(&list->pollfds, i);
            if (pfd->revents) {
                bool close_socket = false;

                SocketCallback* callback = (SocketCallback*)arrayGet(&list->callbacks, i);

                callback->eventFunc(pfd, callback->context, &close_socket);

                if (close_socket) {
                    if (callback->closeFunc) {
                        callback->closeFunc(callback->context);
                    }
                    close(pfd->fd);

                    arraySet(&list->pollfds, i, arrayPop(&list->pollfds));
                    arraySet(&list->callbacks, i, arrayPop(&list->callbacks));
                    i--;
                }
            }
        }
    }
}