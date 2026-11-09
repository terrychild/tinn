#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/types.h"
#include "lib/sys/polling.h"

typedef struct ServerClientInterface ServerClientInterface;
typedef struct ServerInterface ServerInterface;

typedef ServerClientInterface (*ServerConnectedFunc)(void* context, ServerInterface server);

typedef void (*ServerReceivedFunc)(void* context, Slice data);
typedef void (*ServerSentFunc)(void* context);
typedef void (*ServerDisconnectedFunc)(void* context);

struct ServerClientInterface {
    ServerReceivedFunc onReceived;
    ServerSentFunc onSent;
    ServerDisconnectedFunc onDisconnected;
    void* context;
};

void serverClientRecived(ServerClientInterface interface, Slice data);
void serverClientSent(ServerClientInterface interface);
void serverClientDisconnected(ServerClientInterface interface);

typedef void (*ServerReadFunc)(void* context, U64 length);
typedef void (*ServerSendFunc)(void* context, Slice message);
typedef void (*ServerDisconnectFunc)(void* context);

struct ServerInterface {
    Memory* mem;
    ServerReadFunc read;
    ServerSendFunc send;
    ServerDisconnectFunc disconnect;
    void* context;
};

void serverRead(ServerInterface interface, U64 length);
void serverSend(ServerInterface interface, Slice data);
void serverDisconnect(ServerInterface interface);

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
    ServerClientInterface interface;
} ServerConnection;

Server* serverNew(Memory* mem, Polling* polling, const char* port, ServerConnectedFunc onConnect, void* context);

#endif