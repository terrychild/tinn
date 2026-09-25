#include <stdlib.h>

#include "lib/macros.h"
#include "lib/net/server.h"
#include "lib/sys/sockets.h"
#include "lib/mem/allocator.h"
#include "lib/mem/pool.h"
#include "lib/mem/buffer.h"
#include "lib/log.h"

#define CLEAN_POOL 1
#define REMOVE_SOCKET 2
static void connectionClose(ServerConnection* connection, U8 flags);

// connection functions
static ssize_t sendMessage(ServerConnection* connection) {
    ssize_t sent = send(connection->socket, connection->message.start, connection->message.length, MSG_DONTWAIT);
    if (sent >= 0) {
        DEBUG("Sent: %ld/%ld bytes", sent, connection->message.length);
        if ((size_t)sent < connection->message.length) {
            connection->message.start += sent;
            connection->message.length -= sent;
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

void connectionReceive(ServerConnection* connection) {
    bufReset(connection->buffer);
    pollingEvents(connection->server->polling, connection->socket, POLLIN);
}
void connectionSend(ServerConnection* connection, Slice message) {
    connection->message = message;
    if (sendMessage(connection) < 0) {
        connectionClose(connection, CLEAN_POOL | REMOVE_SOCKET);
    }
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
        if (pfd->revents & POLLIN) {
            BufferSpace wrtie_buf = bufReadyWrite(connection->buffer, KB(4));
            ssize_t recvied = recv(pfd->fd, wrtie_buf.start, wrtie_buf.length, 0);
            if (recvied > 0) {
                DEBUG("Recived: %ld bytes", recvied);
                bufConfirmWrite(connection->buffer, recvied);
                if (connection->server->onReceive) {
                    connection->server->onReceive(connection, bufAsSlice(connection->buffer));
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
    deallocateChild(connection->server->allocator, connection->allocator);
    if (flags & CLEAN_POOL) {
        poolRemove(connection->server->connections, connection);
    }
    if (flags & REMOVE_SOCKET) {
        pollingRemove(connection->server->polling, connection->socket);
    }
    LOG("Connection from %s (%d) closed", connection->address, connection->socket);
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
        connection->allocator = allocateChild(server->allocator);
        connection->buffer = bufNew(connection->allocator, KB(4), 0);
        connection->context = server->context;

        if (server->onConnect) {
            server->onConnect(connection);
        }

        pollingAdd(server->polling, connection->socket, (PollingCallback) {
            .func = onConnectionEvent,
            .context = connection
        });

        LOG("Connection from %s (%d) opened", connection->address, connection->socket);
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
    server->allocator = allocator;
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

    server->connections = poolNew(allocator, sizeof(ServerConnection), 256, 0);
    
    server->onConnect = NULL;
    server->onDisconnect = NULL;
    server->onReceive = NULL;
    server->onSent = connectionReceive;

    server->context = NULL;

    return true;
}