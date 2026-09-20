#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "lib/log.h"

int listenToSocket(char* port) {
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

static void *getSocketAddr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int acceptSocket(int fd, char* text_address) {
    struct sockaddr_storage bin_address;
    socklen_t address_size;
    int client_socket;

    address_size = sizeof(bin_address);
    if ((client_socket = accept(fd, (struct sockaddr *)&bin_address, &address_size)) < 0) {
        ERROR("Accepting connection");
        return -1;
    }
    
    inet_ntop(bin_address.ss_family, getSocketAddr((struct sockaddr *)&bin_address), text_address, INET6_ADDRSTRLEN);

    return client_socket;
}