#include <stdlib.h>
#include <string.h>

#include "lib/net/server.h"
#include "lib/net/socket.h"
#include "lib/log.h"

static void onConnectionEvent(struct pollfd* pfd, void* context, bool* close) {
    ServerConnection* connection = context;

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
                DEBUG("%.*s", recvied, buffer);
                if (strncmp(buffer, "quit\r\n", 6)==0) {
                    serverClose(connection->server);
                }
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
    DEBUG("Client socket closed");
    ServerConnection* connection = (ServerConnection*)context;
    if (connection->server->closeConnection) {
        connection->server->closeConnection(connection);
    }
    poolRemove(connection->server->connections, connection);
}
static void closeConnections(Server* server) {    
    PoolNode* node = server->connections->first;
    while (node != NULL) {
        ServerConnection* connection = (ServerConnection*)poolData(node);
        if (server->closeConnection) {
            server->closeConnection(connection);
        }
        socketsRemove(connection->server->sockets, connection->socket);
        LOG("Connection from %s (%d) closed", connection->address, connection->socket);
        node = node->next;
    }
}

static void onServerEvent(struct pollfd* pfd, void* context, __attribute__((unused)) bool* close) {
    Server* server = context;

    if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
        PANIC("Error on server socket: %d", pfd->revents);
    }

    ServerConnection* connection = poolAdd(server->connections);

    connection->socket = acceptSocket(pfd->fd, connection->address);
    if (connection->socket < 0) {
        poolRemove(server->connections, connection);
    } else {
        connection->server = server;
        //TODO:connection->arena = arenaPoolAdd(server->arena_pool, 0);
        
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
    DEBUG("Server socket closed");
    closeConnections((Server*)context);
}
void serverClose(Server* server) {
    DEBUG("Close server");
    closeConnections(server);
    socketsRemove(server->sockets, server->socket);
}

Server* serverNew(Arena* arena, Sockets* sockets, char* port) {
    Server* server = arenaAlloc(arena, sizeof(Server));
    if (!serverInit(server, arena, sockets, port)) {
        return NULL;
    }
    return server;
}
bool serverInit(Server* server, Arena* arena, Sockets* sockets, char* port) {
    server->arena = arena;
    server->sockets = sockets;

    DEBUG("Opening server socket on port %s", port);
    server->socket = listenToSocket(port);
    if (server->socket < 0) {
        return false;
    }

    socketsAdd(sockets, server->socket, (SocketCallback) {
        .eventFunc = onServerEvent,
        .closeFunc = onServerClose,
        .context = server
    });

    server->connections = poolNew(arena, sizeof(ServerConnection), 256, 0);
    
    server->openConnection = NULL;
    server->closeConnection = NULL;
    server->receive = NULL;
    server->send = NULL;
    server->context = NULL;

    return true;
}