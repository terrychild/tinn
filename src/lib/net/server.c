#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lib/net/server.h"
#include "lib/console.h"
#include "lib/mem.h"

static void *getSocketAddr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static void connectionEvent(struct pollfd* pfd, void* context_in, bool* close) {
    SocketServerConnection* context = context_in;

    LOG("connection chatter");

    if (pfd->revents & POLLHUP) {
        LOG("Connection from %s (%d) hung up", context->address, pfd->fd);
        *close = true;
    } else if (pfd->revents & (POLLERR | POLLNVAL)) {
        ERROR("Socket error from %s (%d): %d", context->address, pfd->fd, pfd->revents);
        *close = true;
    } else {
        if (pfd->revents & POLLIN) {
            //flag = read_request(pfd, state);


            char buffer[256];
            int recvied = recv(pfd->fd, buffer, 256, 0);
            if (recvied > 0) {
                DEBUG("recived: %d", recvied);
                DEBUG_DETAIL("%.*s", recvied, buffer);
            } else {
                if (recvied < 0) {
                    ERROR("recv error from %s (%d)", context->address, pfd->fd);
                } else {
                    LOG("Connection from %s (%d) closed", context->address, pfd->fd);
                }
                *close = true;
            }

        } else if (pfd->revents & POLLOUT) {
            //flag = send_response(pfd, state);
        }
    }
}

static void connectionClose(void* context_in) {
    DEBUG("Free client context");
    SocketServerConnection* context = context_in;
    if (context->close) {
        context->close(context_in);
    }
    free(context);
}

static void serverListener(struct pollfd* pfd, void* context_in, bool* close) {
    SocketServer* context = context_in;

    struct sockaddr_storage address;
    socklen_t address_size;
    int client_socket;
    /*ClientState* client_state;
    int client_index;*/

    if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
        PANIC("error on server socket: %d", pfd->revents);
    } 
    
    address_size = sizeof(address);
    if ((client_socket = accept(pfd->fd, (struct sockaddr *)&address, &address_size)) < 0) {
        ERROR("accept");
    } else {
        SocketServerConnection* connection = allocate(NULL, sizeof(*connection)); //TODO: Free!!!
        inet_ntop(address.ss_family, getSocketAddr((struct sockaddr *)&address), connection->address, INET6_ADDRSTRLEN);

        connection->receive = context->receive;
        connection->send = context->send;
        connection->close = context->close;

        if (context->connect) {
            connection->context = context->connect(context->context);
        } else {
            connection->context = context->context;
        }

        socketsAdd(context->sockets, client_socket, (SocketCallback) {
            .eventFunc = connectionEvent,
            .closeFunc = connectionClose,
            .context = connection
        });

        LOG("Connection from %s (%d) opened", connection->address, client_socket);
    }
}
static void serverClose(void* context) {
    DEBUG("Free server context");
    free(context);
}

SocketServer* socketServerNew(Sockets* list, char* port) {
    SocketServer* context = allocate(NULL, sizeof(*context));
    memset(context, 0, sizeof(*context));
    context->sockets = list;

    DEBUG("Opening server socket on port %s", port);
    int server_socket = getServerSocket(port);
    if (server_socket < 0) {
        ERROR("getting server socket");
        return NULL;
    }

    socketsAdd(list, server_socket, (SocketCallback) {
        .eventFunc = serverListener,
        .closeFunc = serverClose,
        .context = context
    });

    return context;
}