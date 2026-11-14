#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "lib/net/server.h"
#include "lib/macros.h"
#include "lib/sys/sockets.h"
#include "lib/mem/memory.h"
#include "lib/mem/pool.h"
#include "lib/mem/buffer.h"
#include "lib/log.h"

// Inbound interface
void serverRecived(ServerInbound interface, Slice data) {
    if (interface.onReceived) {
        interface.onReceived(interface.context, data);
    }
}
void serverSent(ServerInbound interface) {
    if (interface.onSent) {
        interface.onSent(interface.context);
    }
}
void serverDisconnected(ServerInbound interface) {
    if (interface.onDisconnected) {
        interface.onDisconnected(interface.context);
    }
}

// Outbound interface
void serverRead(ServerOutbound interface, U64 length) {
    interface.read(interface.context, length);
}
void serverSend(ServerOutbound interface, Slice data) {
    interface.send(interface.context, data);
}
void serverDisconnect(ServerOutbound interface) {
    interface.disconnect(interface.context);
}

// Server interface
bool serverAddListener(ServerInterface interface, ServerConnectedFunc onConnect, void* context) {
    if (!interface.addListener) {
        return false;
    }
    interface.addListener(interface.context, onConnect, context);
    return true;
}

// Server state
typedef struct {
    Polling* polling;
    int socket;
    Pool* connections;
    U64 connection_size;
    ServerConnectedFunc onConnect;
    void* context;
} Server;

typedef struct {
    int socket;
    char address[INET6_ADDRSTRLEN];
    Server* server;
    Memory* mem;
    Buffer* request;
    Slice response;
    ServerInbound interface;
} Connection;

// Connection events
static void connectionClose(Connection* connection, bool remove_from_pool) {
    serverDisconnected(connection->interface);
    pollingRemove(connection->server->polling, connection->socket);
    memoryRelease(connection->mem);
    close(connection->socket);
    LOG("Connection from %s (%d) closed", connection->address, connection->socket);
    if (remove_from_pool) {
        poolRemove(connection->server->connections, connection);
    }
}

static void onConnectionDisconnect(void* context) {
    Connection* connection = context;
    connectionClose(connection, true);
}

static ssize_t connectionSend(Connection* connection) {
    ssize_t sent = send(connection->socket, connection->response.start, connection->response.length, MSG_DONTWAIT);
    if (sent >= 0) {
        DEBUG("Sent: %ld/%ld bytes", sent, connection->response.length);
        if ((size_t)sent < connection->response.length) {
            connection->response.start += sent;
            connection->response.length -= sent;
            pollingEvents(connection->server->polling, connection->socket, POLLOUT);//TODO: make this and add and not a set?
        } else {
            pollingEvents(connection->server->polling, connection->socket, POLLIN);
            serverSent(connection->interface);
        }
    } else {
        ERROR("send error for %s (%d)", connection->address, connection->socket);
    }
    return sent;
}

static void onConnectionSend(void* context, Slice response) {
    Connection* connection = context;
    connection->response = response;
    if (connectionSend(connection) < 0) {
        connectionClose(connection, true);
    }
}

static void onConnectionRead(void* context, U64 length) {
    Connection* connection = context;
    bufConsume(connection->request, length);
}

static void onConnectionEvent(struct pollfd* pfd, void* context) {
    Connection* connection = context;

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
                serverRecived(connection->interface, bufAsSlice(connection->request));
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

// Server events
static void onServerEvent(struct pollfd* pfd, void* context) {
    Server* server = context;

    if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
        PANIC("Error on server socket: %d", pfd->revents);
    }

    Connection* connection = poolAdd(server->connections);

    connection->socket = acceptSocket(pfd->fd, connection->address);
    if (connection->socket < 0) {
        poolRemove(server->connections, connection);
    } else {
        connection->server = server;
        connection->mem = memoryNew(server->connection_size);
        connection->request = bufNew(connection->mem, KiB(4));

        connection->interface = server->onConnect(server->context, (ServerOutbound) {
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
        Connection* connection = (Connection*)node->data;
        connectionClose(connection, false);
        node = node->next;
    }
    pollingRemove(server->polling, server->socket);
}

// Server setup
void addListener(void* server_context, ServerConnectedFunc onConnect, void* context) {
    Server* server = server_context;
    server->onConnect = onConnect;
    server->context = context;
}

ServerInterface serverNew(Memory* mem, Polling* polling, const char* port) {
    Server* server = allocate(mem, sizeof(Server));
    server->polling = polling;

    DEBUG("Opening server socket on port %s", port);
    server->socket = listenToSocket(port);
    if (server->socket < 0) {
        return (ServerInterface) {0};
    }

    pollingAdd(server->polling, server->socket, (PollingCallback) {
        .func = onServerEvent,
        .context = server
    });

    server->connections = poolNew(mem, sizeof(Connection), 256);
    server->connection_size = MiB(100);

    server->onConnect = NULL;
    server->context = NULL;

    return (ServerInterface) {
        .addListener = addListener,
        .context = server,
    };
}