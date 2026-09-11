#include <stdlib.h>
#include <unistd.h>

#include "lib/net/sockets.h"
#include "lib/console.h"

void socketsInit(Sockets* list, U64 max_capacity, ArenaPool* pool) {
    U64 size = 8;
    max_capacity = max_capacity ? max_capacity : 256;

    poolInit(&list->pollfds, sizeof(struct pollfd), size, max_capacity, pool);
    poolInit(&list->callbacks, sizeof(SocketCallback), size, max_capacity, pool);
}

void socketsRelease(Sockets* list) {
    poolRelease(&list->pollfds);
    poolRelease(&list->callbacks);
}

void socketsAdd(Sockets* list, int new_socket, SocketCallback callback) {
    poolAdd(&list->pollfds, &(struct pollfd){
        .fd = new_socket,
        .events = POLLIN,
        .revents = 0
    });
    poolAdd(&list->callbacks, &callback);
}

void socketsPoll(Sockets* list) {
    while (list->pollfds.count > 0) {
        if (poll((struct pollfd*)list->pollfds.array.data, list->pollfds.count, -1) < 0 ) {
            PANIC("When polling");
        }

        for (U64 i = 0; i < list->pollfds.count; i++) {
            struct pollfd* pfd = (struct pollfd*)poolGet(&list->pollfds, i);
            if (pfd->revents) {
                bool close_socket = false;

                SocketCallback* callback = (SocketCallback*)poolGet(&list->callbacks, i);

                callback->eventFunc(pfd, callback->context, &close_socket);

                if (close_socket) {
                    if (callback->closeFunc) {
                        callback->closeFunc(callback->context);
                    }
                    close(pfd->fd);

                    poolRemove(&list->pollfds, i);
                    poolRemove(&list->callbacks, i);
                    i--;
                }
            }
        }
    }
}