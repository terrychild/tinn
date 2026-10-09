#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/types.h"
#include "lib/sys/polling.h"

typedef struct ServerConnection ServerConnection;
typedef struct ServerExchange ServerExchange;

typedef void (*ServerConnectFunc)(ServerConnection* connection);
typedef void (*ServerDisconnectFunc)(ServerConnection* connection);
typedef void (*ServerReceiveFunc)(ServerConnection* connection, Slice data);
typedef void (*ServerSentFunc)(ServerConnection* connection, bool* remove);

typedef struct {
    Allocator* scope;
    Polling* polling;
    int socket;
    Pool* connections;
    U64 connection_size;
    U64 exchange_size;
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
    Allocator* scope;
    Allocator* exchange_scope;
    ServerExchange* exchange;
    void* context;
};

struct ServerExchange {
    ServerConnection* connection;
    Buffer* request;
    Slice response;
};

Server* serverNew(Allocator* allocator, Polling* polling, const char* port);
bool serverInit(Server* server, Allocator* allocator, Polling* polling, const char* port);
void serverClose(Server* server);

ServerExchange* connectionStartExchange(ServerConnection* connection);
void connectionEndExchange(ServerConnection* connection);

void connectionReceive(ServerConnection* connection);
void connectionSend(ServerConnection* connection, Slice message);
void connectionSent(ServerConnection* connection, bool* remove);

#endif