#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>

#include "lib/net/sockets.h"
#include "lib/console.h"
#include "lib/mem.h"

// Generic Socket stuff

int getServerSocket(char* port) {
    int status;

    struct addrinfo hints;
    struct addrinfo* addresses;
    struct addrinfo* address;

    int sock;

    // get local address info
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, port, &hints, &addresses)) != 0) {
        ERROR("gettaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    // get a socket and bind to it
    for (address = addresses; address != NULL; address = address->ai_next) {
        if ((sock = socket(address->ai_family, address->ai_socktype, address->ai_protocol)) < 0) {
            continue;
        }

        // re-use socket if in use
        int yes=1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(sock, address->ai_addr, address->ai_addrlen) != 0) {
            close(sock);
            continue;
        }

        break;
    }

    // if address is null nothing bound
    if (address == NULL) {
        freeaddrinfo(addresses);
        ERROR("unable to bind to a socket");
        return -1;
    }

    freeaddrinfo(addresses);

    // listen to socket
    if (listen(sock, 10) != 0) {
        ERROR("unable to listen to a socket");
        return -1;
    }

    return sock;
}

// Socket List managment

void socketsInit(Sockets* list) {
    list->size = 8;
    list->count = 0;
    list->pollfds = allocate(NULL, sizeof(*list->pollfds) * list->size);
    list->callbacks = allocate(NULL, sizeof(*list->callbacks) * list->size);
}

void socketsFree(Sockets* list) {
    free(list->pollfds);
    free(list->callbacks);
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
            PANIC("when polling");
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