#include <stdlib.h>
#include <unistd.h>

#include "lib/net/server.h"
#include "lib/macros.h"
#include "lib/sys/sockets.h"
#include "lib/mem/memory.h"
#include "lib/mem/pool.h"
#include "lib/mem/buffer.h"
#include "lib/log.h"

// interface helpers
void serverClientRecived(ServerClientInterface interface, Slice data) {
    if (interface.onReceived) {
        interface.onReceived(interface.context, data);
    }
}
void serverClientSent(ServerClientInterface interface) {
    if (interface.onSent) {
        interface.onSent(interface.context);
    }
}
void serverClientDisconnected(ServerClientInterface interface) {
    if (interface.onDisconnected) {
        interface.onDisconnected(interface.context);
    }
}

void serverRead(ServerInterface interface, U64 length) {
    interface.read(interface.context, length);
}
void serverSend(ServerInterface interface, Slice data) {
    interface.send(interface.context, data);
}
void serverDisconnect(ServerInterface interface) {
    interface.disconnect(interface.context);
}

// connection events
static void connectionClose(ServerConnection* connection, bool remove_from_pool) {
    serverClientDisconnected(connection->interface);
    pollingRemove(connection->server->polling, connection->socket);
    memoryRelease(connection->mem);
    close(connection->socket);
    LOG("Connection from %s (%d) closed", connection->address, connection->socket);
    if (remove_from_pool) {
        poolRemove(connection->server->connections, connection);
    }
}

static void onConnectionDisconnect(void* context) {
    ServerConnection* connection = context;
    connectionClose(connection, true);
}

static ssize_t connectionSend(ServerConnection* connection) {
    ssize_t sent = send(connection->socket, connection->response.start, connection->response.length, MSG_DONTWAIT);
    if (sent >= 0) {
        DEBUG("Sent: %ld/%ld bytes", sent, connection->response.length);
        if ((size_t)sent < connection->response.length) {
            connection->response.start += sent;
            connection->response.length -= sent;
            pollingEvents(connection->server->polling, connection->socket, POLLOUT);
        } else {
            serverClientSent(connection->interface);
        }
    } else {
        ERROR("send error for %s (%d)", connection->address, connection->socket);
    }
    return sent;
}

static void onConnectionSend(void* context, Slice response) {
    ServerConnection* connection = context;
    connection->response = response;
    if (connectionSend(connection) < 0) {
        connectionClose(connection, true);
    }
}

static void onConnectionRead(void* context, U64 length) {
    ServerConnection* connection = context;
    bufConsume(connection->request, length);
}

static void onConnectionEvent(struct pollfd* pfd, void* context) {
    ServerConnection* connection = context;

    if (pfd->revents & POLLHUP) {
        LOG("Connection from %s (%d) hung up", connection->address, pfd->fd);
        connectionClose(connection, true);
    } else if (pfd->revents & (POLLERR | POLLNVAL)) {
        ERROR("Socket error from %s (%d): %d", connection->address, pfd->fd, pfd->revents);
        connectionClose(connection, true);
    } else {
        if (pfd->revents & POLLIN) {
            BufferSpace wrtie_buf = bufReadyWrite(connection->request, KiB(1));
            ssize_t recvied = recv(pfd->fd, wrtie_buf.start, wrtie_buf.length, 0);
            if (recvied > 0) {
                DEBUG("Recived: %ld bytes", recvied);
                bufConfirmWrite(connection->request, recvied);
                serverClientRecived(connection->interface, bufAsSlice(connection->request));
            } else {
                if (recvied < 0) {
                    ERROR("recv error from %s (%d)", connection->address, pfd->fd);
                }
                connectionClose(connection, true);
            }

        } else if (pfd->revents & POLLOUT) {
            if (connectionSend(connection) < 0) {
                connectionClose(connection, true);
            }
        }
    }
}

// server events
static void onServerEvent(struct pollfd* pfd, void* context) {
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
        connection->mem = memoryNew(server->connection_size);
        connection->request = bufNew(connection->mem, KiB(4));

        connection->interface = server->onConnect(server->context, (ServerInterface) {
            .mem = connection->mem,
            .read = onConnectionRead,
            .send = onConnectionSend,
            .disconnect = onConnectionDisconnect,
            .context = connection
        });

        pollingAdd(server->polling, connection->socket, (PollingCallback) {
            .func = onConnectionEvent,
            .context = connection
        });

        LOG("Connection from %s (%d) opened", connection->address, connection->socket);
    }
}

void serverClose(Server* server) {
    DEBUG("Close server");
    PoolNode* node = server->connections->first;
    while (node != NULL) {
        ServerConnection* connection = (ServerConnection*)node->data;
        connectionClose(connection, false);
        node = node->next;
    }
    pollingRemove(server->polling, server->socket);
}

// server setup
Server* serverNew(Memory* mem, Polling* polling, const char* port, ServerConnectedFunc onConnect, void* context) {
    Server* server = allocate(mem, sizeof(Server));
    server->polling = polling;

    DEBUG("Opening server socket on port %s", port);
    server->socket = listenToSocket(port);
    if (server->socket < 0) {
        return NULL;
    }

    pollingAdd(server->polling, server->socket, (PollingCallback) {
        .func = onServerEvent,
        .context = server
    });

    server->connections = poolNew(mem, sizeof(ServerConnection), 256);
    server->connection_size = MiB(100);

    server->onConnect = onConnect;
    server->context = context;

    return server;
}