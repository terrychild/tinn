#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/types.h"
#include "lib/mem/buffer.h"
#include "lib/net/sockets.h"

typedef struct ServerConnection ServerConnection;

typedef void (*ServerConnectFunc)(ServerConnection* connection);
typedef void (*ServerDisconnectFunc)(ServerConnection* connection);
typedef void (*ServerReceiveFunc)(ServerConnection* connection, Slice data);
typedef void (*ServerSentFunc)(ServerConnection* connection);

typedef struct {
    Allocator* allocator;
    Sockets* sockets;
    struct pollfd* socket;
    Pool* connections;
    ServerConnectFunc onConnect;
    ServerDisconnectFunc onDisconnect;
    ServerReceiveFunc onReceive;
    ServerSentFunc onSent;
    void* context;
} Server;

struct ServerConnection {
    char address[INET6_ADDRSTRLEN];
    struct pollfd* socket;
    Server* server;
    Allocator* allocator;
    Buffer* buffer;
    Slice message;
    void* context;
};

Server* serverNew(Allocator* allocator, Sockets* sockets, char* port);
bool serverInit(Server* server, Allocator* allocator, Sockets* sockets, char* port);
void serverClose(Server* server);

void connectionReceive(ServerConnection* connection);
void connectionSend(ServerConnection* connection, Slice message);

#endif