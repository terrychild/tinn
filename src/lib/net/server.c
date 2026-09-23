#include <stdlib.h>
#include <string.h>

#include "lib/macros.h"
#include "lib/net/server.h"
#include "lib/net/socket.h"
#include "lib/mem/allocator.h"
#include "lib/mem/pool.h"
#include "lib/mem/buffer.h"
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

            BufferSpace wrtie_buf = bufReadyWrite(connection->buf_in, KB(4));
            ssize_t recvied = recv(pfd->fd, wrtie_buf.start, wrtie_buf.length, 0);
            if (recvied > 0) {
                bufConfirmWrite(connection->buf_in, recvied);
                DEBUG("Recived: %ld bytes", recvied);
                bufHexDump(connection->buf_in);
                allocatorDebug(connection->server->allocator);
                if (strncmp(bufAsStr(connection->buf_in), "quit\r\n", 6)==0) {
                    serverClose(connection->server);
                } else {
                    connection->data_out = bufAsSlice(connection->buf_in);
                    ssize_t sent = send(pfd->fd, connection->data_out.start, connection->data_out.length, MSG_DONTWAIT);
                    if (sent >= 0) {
                        DEBUG("Sent: %ld/%ld bytes", sent, connection->data_out.length);
                        if ((size_t)sent < connection->data_out.length) {
                            connection->data_out.start += sent;
                            connection->data_out.length -= sent;
                            pfd->events = POLLOUT;
                        } else {
                            bufReset(connection->buf_in);
                            pfd->events = POLLIN;
                        }
                    } else {
                        ERROR("send error for %s (%d)", connection->address, pfd->fd);
                        *close = true;
                    }
                }     
            }
            /*char buffer[256];
            int recvied = recv(pfd->fd, buffer, 256, 0);
            if (recvied > 0) {
                DEBUG("recived: %d", recvied);
                DEBUG("%.*s", recvied, buffer);
                if (strncmp(buffer, "quit\r\n", 6)==0) {
                    serverClose(connection->server);
                }
            }*/ else {
                if (recvied < 0) {
                    ERROR("recv error from %s (%d)", connection->address, pfd->fd);
                } else {
                    LOG("Connection from %s (%d) closed", connection->address, pfd->fd);
                }
                *close = true;
            }

        } else if (pfd->revents & POLLOUT) {
            //flag = send_response(pfd, state);
            ssize_t sent = send(pfd->fd, connection->data_out.start, connection->data_out.length, MSG_DONTWAIT);
            if (sent >= 0) {
                DEBUG("Sent: %ld/%ld bytes", sent, connection->data_out.length);
                if ((size_t)sent < connection->data_out.length) {
                    connection->data_out.start += sent;
                    connection->data_out.length -= sent;
                    pfd->events = POLLOUT;
                } else {
                    bufReset(connection->buf_in);
                    pfd->events = POLLIN;
                }
            } else {
                ERROR("send error for %s (%d)", connection->address, pfd->fd);
                *close = true;
            }
        }
    }
}

static void connectionClose(ServerConnection* connection) {
    if (connection->server->closeConnection) {
        connection->server->closeConnection(connection);
    }
    deallocateChild(connection->server->allocator, connection->allocator);
}
static void onConnectionClose(void* context) {
    DEBUG("Client socket closed");
    ServerConnection* connection = (ServerConnection*)context;
    connectionClose(connection);
    poolRemove(connection->server->connections, connection);
}
static void connectionsCloseAll(Server* server) {    
    PoolNode* node = server->connections->first;
    while (node != NULL) {
        ServerConnection* connection = (ServerConnection*)poolData(node);
        connectionClose(connection);
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
        connection->allocator = allocateChild(server->allocator);
        connection->buf_in = bufNew(connection->allocator, KB(4), 0);
        
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
    connectionsCloseAll((Server*)context);
}
void serverClose(Server* server) {
    DEBUG("Close server");
    connectionsCloseAll(server);
    socketsRemove(server->sockets, server->socket);
}

Server* serverNew(Allocator* allocator, Sockets* sockets, char* port) {
    Server* server = allocate(allocator, sizeof(Server));
    if (!serverInit(server, allocator, sockets, port)) {
        return NULL;
    }
    return server;
}
bool serverInit(Server* server, Allocator* allocator, Sockets* sockets, char* port) {
    server->allocator = allocator;
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

    server->connections = poolNew(allocator, sizeof(ServerConnection), 256, 0);
    
    server->openConnection = NULL;
    server->closeConnection = NULL;
    server->receive = NULL;
    server->send = NULL;
    server->context = NULL;

    return true;
}