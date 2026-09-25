#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/types.h"
#include "lib/sys/polling.h"

typedef struct ServerConnection ServerConnection;

typedef void (*ServerConnectFunc)(ServerConnection* connection);
typedef void (*ServerDisconnectFunc)(ServerConnection* connection);
typedef void (*ServerReceiveFunc)(ServerConnection* connection, Slice data);
typedef void (*ServerSentFunc)(ServerConnection* connection);

typedef struct {
    Allocator* allocator;
    Polling* polling;
    int socket;
    Pool* connections;
    ServerConnectFunc onConnect;
    ServerDisconnectFunc onDisconnect;
    ServerReceiveFunc onReceive;
    ServerSentFunc onSent;
    void* context;
} Server;

struct ServerConnection {
    int socket;
    char address[INET6_ADDRSTRLEN];
    Server* server;
    Allocator* allocator;
    Buffer* buffer;
    Slice message;
    void* context;
};

Server* serverNew(Allocator* allocator, Polling* polling, const char* port);
bool serverInit(Server* server, Allocator* allocator, Polling* polling, const char* port);
void serverClose(Server* server);

void connectionReceive(ServerConnection* connection);
void connectionSend(ServerConnection* connection, Slice message);

#endif