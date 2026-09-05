#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>

#include "lib/net.h"
#include "lib/console.h"
#include "lib/mem.h"

int getLocalSocket(char* port) {
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

void socketsInit(Sockets* list) {
    list->size = 8;
    list->count = 0;
    list->sockets = allocate(NULL, sizeof(*list->sockets) * list->size);
}

void socketsFree(Sockets* list) {
    free(list->sockets);
}

void socketsAdd(Sockets* list, int new_socket, SocketListener new_listener, void* new_state) {
    // do we need to expand the arrays
    if (list->count == list->size) {
        list->size *= 2;
        list->sockets = allocate(list->sockets, sizeof(*list->sockets) * list->size);
    }

    // add new socket
    list->sockets[list->count].pollfd.fd = new_socket;
    list->sockets[list->count].pollfd.events = POLLIN;
    list->sockets[list->count].pollfd.revents = 0;

    list->sockets[list->count].listener = new_listener;

    list->sockets[list->count].state = new_state;

    // update count
    list->count++;
}

void socketsRm(Sockets* list, U64 index) {
    if (index < list->count) {
        if (index < list->count-1) {
            list->sockets[index] = list->sockets[list->count-1];
        }
        list->count--;
    }
}