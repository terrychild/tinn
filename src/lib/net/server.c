#include <stdlib.h>
#include <string.h>

#include "lib/net/socket.h"
#include "lib/net/sockets.h"
#include "lib/net/server.h"
#include "lib/mem/heap.h"
#include "lib/console.h"

static void onConnectionEvent(struct pollfd* pfd, void* context_in, bool* close) {
    ServerConnection* context = context_in;

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

static void onConnectionClose(void* context_in) {
    DEBUG("Free client context");
    ServerConnection* context = context_in;
    if (context->close) {
        context->close(context_in);
    }
    free(context);
}

static void onServerEvent(struct pollfd* pfd, void* context_in, __attribute__((unused)) bool* close) {
    Server* context = context_in;

    if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
        PANIC("Error on server socket: %d", pfd->revents);
    }

    ServerConnection* connection = allocate(NULL, sizeof(*connection));
    int client_socket = acceptSocket(pfd->fd, connection->address);
    if (client_socket < 0) {
        free(connection);
    } else {
        connection->receive = context->receive;
        connection->send = context->send;
        connection->close = context->close;

        if (context->connect) {
            connection->context = context->connect(context->context);
        } else {
            connection->context = context->context;
        }

        socketsAdd(context->sockets, client_socket, (SocketCallback) {
            .eventFunc = onConnectionEvent,
            .closeFunc = onConnectionClose,
            .context = connection
        });

        LOG("Connection from %s (%d) opened", connection->address, client_socket);
    }
}
static void onServerClose(void* context) {
    DEBUG("Free server context");
    free(context);
}

Server* serverNew(Sockets* list, char* port) {
    Server* context = allocate(NULL, sizeof(*context));
    memset(context, 0, sizeof(*context));
    context->sockets = list;

    DEBUG("Opening server socket on port %s", port);
    int server_socket = listenToSocket(port);
    if (server_socket < 0) {
        return NULL;
    }

    socketsAdd(list, server_socket, (SocketCallback) {
        .eventFunc = onServerEvent,
        .closeFunc = onServerClose,
        .context = context
    });

    return context;
}