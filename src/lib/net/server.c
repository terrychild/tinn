#include <stdlib.h>
#include <string.h>

#include "lib/macros.h"
#include "lib/net/socket.h"
#include "lib/net/sockets.h"
#include "lib/net/server.h"
#include "lib/mem/arena-pool.h"
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

            ssize_t recvied = recv(pfd->fd, stringWritePtr(&connection->buffer_in), stringWriteMax(&connection->buffer_in, KB(4)), 0);
            if (recvied > 0) {
                DEBUG("recived: %d", recvied);
                DEBUG_DETAIL("%.*s", recvied, connection->buffer_in.data);
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

static void connectionClose(ServerConnection* connection) {
    DEBUG("Close connection");
    if (connection->server->closeConnection) {
        connection->server->closeConnection(connection);
    }
    stringRelease(&connection->buffer_in);
    poolRemove(&connection->server->connections, connection);
}

static void onConnectionClose(void* context) {
    connectionClose((ServerConnection*)context);
}

static void onServerEvent(struct pollfd* pfd, void* context, __attribute__((unused)) bool* close) {
    Server* server = context;

    if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
        PANIC("Error on server socket: %d", pfd->revents);
    }

    ServerConnection* connection = poolAdd(&server->connections);

    connection->socket = acceptSocket(pfd->fd, connection->address);
    if (connection->socket < 0) {
        poolRemove(&server->connections, connection);
    } else {
        connection->server = server;
        stringInit(&connection->buffer_in, server->arena_pool, 0, 0);
        
        if (server->openConnection) {
            connection->context = server->openConnection(server->context);
        } else {
            connection->context = server->context;
        }
        
        socketsAdd(server->sockets, connection->socket, (SocketCallback) {
            .eventFunc = onConnectionEvent,
            .closeFunc = onConnectionClose,
            .context = connection
        });

        LOG("Connection from %s (%d) opened", connection->address, connection->socket);
    }
}
static void onServerClose(void* context) {
    DEBUG("Close server"); 
    //TODO Test!
    Server* server = context;

    PoolNode* node = server->connections.used;
    while (node != NULL) {
        ServerConnection* connection = (ServerConnection*)(node + 1);
        connectionClose(connection);
        socketsRemove(connection->server->sockets, connection->socket);
        node = node->next;
    }

    poolRelease(&server->connections);
}

bool serverInit(Server* server, ArenaPool* arena_pool, Sockets* sockets, char* port) {
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

    poolInit(&server->connections, arena_pool, sizeof(ServerConnection), 256, 0);

    server->openConnection = NULL;
    server->closeConnection = NULL;
    server->receive = NULL;
    server->send = NULL;
    server->context = NULL;

    return true;
}