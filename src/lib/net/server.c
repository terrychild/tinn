#include <stdlib.h>

#include "lib/net/server.h"
#include "lib/macros.h"
#include "lib/sys/sockets.h"
#include "lib/mem/allocator.h"
#include "lib/mem/pool.h"
#include "lib/mem/buffer.h"
#include "lib/log.h"

static const U8 CLEAN_POOL = 1;
static const U8 REMOVE_SOCKET = 2;
static void connectionClose(ServerConnection* connection, U8 flags);

// connection functions
ServerExchange* connectionStartExchange(ServerConnection* connection) {
    connectionEndExchange(connection);
    Allocator* allocator = allocateChild(connection->scope);
    connection->exchange = allocate(allocator, sizeof(ServerExchange));
    connection->exchange->connection = connection;
    connection->exchange->scope = allocator;
    connection->exchange->buffer = bufNew(allocator, KB(4));
    return connection->exchange;
}
void connectionEndExchange(ServerConnection* connection) {
    if (connection->exchange) {
        allocatorReset(connection->exchange->scope);
        connection->exchange = NULL;
    }
}

static ssize_t sendMessage(ServerConnection* connection) {
    ssize_t sent = send(connection->socket, connection->exchange->message.start, connection->exchange->message.length, MSG_DONTWAIT);
    if (sent >= 0) {
        DEBUG("Sent: %ld/%ld bytes", sent, connection->exchange->message.length);
        if ((size_t)sent < connection->exchange->message.length) {
            connection->exchange->message.start += sent;
            connection->exchange->message.length -= sent;
            pollingEvents(connection->server->polling, connection->socket, POLLOUT);
        } else {
            if (connection->server->onSent) {
                connection->server->onSent(connection);
            }
        }
    } else {
        ERROR("send error for %s (%d)", connection->address, connection->socket);
    }
    return sent;
}

void connectionSend(ServerConnection* connection, Slice message) {
    if (!connection->exchange) {
        connectionStartExchange(connection);
    }
    connection->exchange->message = message;
    if (sendMessage(connection) < 0) {
        connectionClose(connection, CLEAN_POOL | REMOVE_SOCKET);
    }
}
void connectionSent(ServerConnection* connection) {
    connectionEndExchange(connection);
    pollingEvents(connection->server->polling, connection->socket, POLLIN);
}

// connection events
static void onConnectionEvent(struct pollfd* pfd, void* context, bool* close) {
    ServerConnection* connection = context;

    if (pfd->revents & POLLHUP) {
        LOG("Connection from %s (%d) hung up", connection->address, pfd->fd);
        connectionClose(connection, CLEAN_POOL);
        *close = true;
    } else if (pfd->revents & (POLLERR | POLLNVAL)) {
        ERROR("Socket error from %s (%d): %d", connection->address, pfd->fd, pfd->revents);
        connectionClose(connection, CLEAN_POOL);
        *close = true;
    } else {
        if (!connection->exchange) {
            connectionStartExchange(connection);
        }
        if (pfd->revents & POLLIN) {
            BufferSpace wrtie_buf = bufReadyWrite(connection->exchange->buffer, KB(1));
            ssize_t recvied = recv(pfd->fd, wrtie_buf.start, wrtie_buf.length, 0);
            if (recvied > 0) {
                DEBUG("Recived: %ld bytes", recvied);
                bufConfirmWrite(connection->exchange->buffer, recvied);
                if (connection->server->onReceive) {
                    connection->server->onReceive(connection, bufAsSlice(connection->exchange->buffer));
                }
            } else {
                if (recvied < 0) {
                    ERROR("recv error from %s (%d)", connection->address, pfd->fd);
                } else {
                    LOG("Connection from %s (%d) closed", connection->address, pfd->fd);
                }
                connectionClose(connection, CLEAN_POOL);
                *close = true;
            }

        } else if (pfd->revents & POLLOUT) {
            if (sendMessage(connection) < 0) {
                connectionClose(connection, CLEAN_POOL);
                *close = true;
            }
        }
    }
}

static void connectionClose(ServerConnection* connection, U8 flags) {
    if (connection->server->onDisconnect) {
        connection->server->onDisconnect(connection);
    }
    deallocateChild(connection->server->scope, connection->scope);
    if (flags & CLEAN_POOL) {
        poolRemove(connection->server->connections, connection);
    }
    if (flags & REMOVE_SOCKET) {
        pollingRemove(connection->server->polling, connection->socket);
    }
    LOG("Connection from %s (%d) closed", connection->address, connection->socket);
    allocatorDebug(connection->server->scope);
}
static void connectionsCloseAll(Server* server) {
    PoolNode* node = server->connections->first;
    while (node != NULL) {
        ServerConnection* connection = (ServerConnection*)poolData(node);
        connectionClose(connection, REMOVE_SOCKET);
        node = node->next;
    }
}

// server events
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
        connection->scope = allocateChild(server->scope);
        connection->exchange = NULL;
        connection->context = server->context;

        if (server->onConnect) {
            server->onConnect(connection);
        }

        pollingAdd(server->polling, connection->socket, (PollingCallback) {
            .func = onConnectionEvent,
            .context = connection
        });

        LOG("Connection from %s (%d) opened", connection->address, connection->socket);
        allocatorDebug(server->scope);
    }
}

void serverClose(Server* server) {
    DEBUG("Close server");
    connectionsCloseAll(server);
    pollingRemove(server->polling, server->socket);
}

// server setup
Server* serverNew(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = allocate(allocator, sizeof(Server));
    if (!serverInit(server, allocator, polling, port)) {
        return NULL;
    }
    return server;
}
bool serverInit(Server* server, Allocator* allocator, Polling* polling, const char* port) {
    server->scope = allocator;
    server->polling = polling;

    DEBUG("Opening server socket on port %s", port);
    server->socket = listenToSocket(port);
    if (server->socket < 0) {
        return false;
    }

    pollingAdd(server->polling, server->socket, (PollingCallback) {
        .func = onServerEvent,
        .context = server
    });

    server->connections = poolNew(server->scope, sizeof(ServerConnection), 256);

    server->onConnect = NULL;
    server->onDisconnect = NULL;
    server->onReceive = NULL;
    server->onSent = connectionSent;

    server->context = NULL;

    return true;
}