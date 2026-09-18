#include <stdlib.h>
#include <string.h>

#include "lib/net/socket.h"
#include "lib/net/sockets.h"
#include "lib/net/server.h"
#include "lib/mem/arena-pool.h"
#include "lib/mem/arena.h"
#include "lib/console.h"

static void onConnectionEvent(struct pollfd* pfd, void* context, bool* close) {
    ServerConnection* connection = context;

    LOG("connection chatter");

    if (pfd->revents & POLLHUP) {
        LOG("Connection from %s (%d) hung up", connection->address, pfd->fd);
        *close = true;
    } else if (pfd->revents & (POLLERR | POLLNVAL)) {
        ERROR("Socket error from %s (%d): %d", connection->address, pfd->fd, pfd->revents);
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
                    ERROR("recv error from %s (%d)", connection->address, pfd->fd);
                } else {
                    LOG("Connection from %s (%d) closed", connection->address, pfd->fd);
                }
                *close = true;
            }

        } else if (pfd->revents & POLLOUT) {
            //flag = send_response(pfd, state);
        }
    }
}

static void onConnectionClose(void* context) {
    DEBUG("Close connection");
    ServerConnection* connection = context;
    if (connection->server->closeConnection) {
        connection->server->closeConnection(context);
    }
    arenaPoolRemove(connection->server->arena_pool, connection->arena);
}

static void onServerEvent(struct pollfd* pfd, void* context, __attribute__((unused)) bool* close) {
    Server* server = context;

    if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
        PANIC("Error on server socket: %d", pfd->revents);
    }

    Arena* arena = arenaPoolAdd(server->arena_pool, 0);
    ServerConnection* connection = arenaAlloc(arena, sizeof(*connection));

    int client_socket = acceptSocket(pfd->fd, connection->address);
    if (client_socket < 0) {
        arenaPoolRemove(server->arena_pool, arena);
    } else {
        connection->server = server;
        connection->arena = arena;

        if (server->openConnection) {
            connection->context = server->openConnection(server->context);
        } else {
            connection->context = server->context;
        }

        socketsAdd(server->sockets, client_socket, (SocketCallback) {
            .eventFunc = onConnectionEvent,
            .closeFunc = onConnectionClose,
            .context = connection
        });

        LOG("Connection from %s (%d) opened", connection->address, client_socket);
    }
}
static void onServerClose(void* context) {
    DEBUG("Close server");
    //TODO: release connections
}

bool serverInit(Server* server, Sockets* sockets, char* port, ArenaPool* arena_pool) {
    server->arena_pool = arena_pool;
    server->sockets = sockets;

    DEBUG("Opening server socket on port %s", port);
    int server_socket = listenToSocket(port);
    if (server_socket < 0) {
        return false;
    }

    socketsAdd(sockets, server_socket, (SocketCallback) {
        .eventFunc = onServerEvent,
        .closeFunc = onServerClose,
        .context = server
    });

    return true;
}