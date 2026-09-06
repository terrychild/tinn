#include <stdlib.h>
#include <unistd.h>

#include "lib/net/sockets.h"
#include "lib/mem.h"
#include "lib/console.h"

Sockets* socketsNew() {
    Sockets* list = allocate(NULL, sizeof(*list));

    list->size = 8;
    list->count = 0;
    list->pollfds = allocate(NULL, sizeof(*list->pollfds) * list->size);
    list->callbacks = allocate(NULL, sizeof(*list->callbacks) * list->size);

    return list;
}

void socketsRelease(Sockets* list) {
    if (list != NULL) {
        free(list->pollfds);
        free(list->callbacks);
        free(list);
    }
}

void socketsAdd(Sockets* list, int new_socket, SocketCallback callback) {
    // do we need to expand the arrays
    if (list->count == list->size) {
        list->size *= 2;
        list->pollfds = allocate(list->pollfds, sizeof(*list->pollfds) * list->size);
        list->callbacks = allocate(list->callbacks, sizeof(*list->callbacks) * list->size);
    }

    // add new socket
    list->pollfds[list->count].fd = new_socket;
    list->pollfds[list->count].events = POLLIN;
    list->pollfds[list->count].revents = 0;

    list->callbacks[list->count] = callback;

    // update count
    list->count++;
}

void socketsPoll(Sockets* list) {
    while (list->count > 0) {
        if (poll(list->pollfds, list->count, -1) < 0 ) {
            PANIC("When polling");
        }

        for (U64 i = 0; i < list->count; i++) {
            if (list->pollfds[i].revents) {
                bool close_socket = false;

                list->callbacks[i].eventFunc(&list->pollfds[i], list->callbacks[i].context, &close_socket);

                if (close_socket) {
                    if (list->callbacks[i].closeFunc) {
                        list->callbacks[i].closeFunc(list->callbacks[i].context);
                    }
                    close(list->pollfds[i].fd);

                    if (i < list->count-1) {
                        list->pollfds[i] = list->pollfds[list->count-1];
                        list->callbacks[i] = list->callbacks[list->count-1];
                    }
                    list->count--;
                    i--;
                }
            }
        }
    }
}