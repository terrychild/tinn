#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/types.h"
#include "lib/net/sockets.h"

typedef void* (*SocketOpenFunc)(void* context);
typedef void (*SocketReceiveFunc)(void* context);
typedef void (*SocketSendFunc)(void* context);

typedef struct {
    Allocator* allocator;
    Sockets* sockets;
    int socket;
    Pool* connections;
    SocketOpenFunc openConnection;
    SocketCloseFunc closeConnection;
    SocketReceiveFunc receive;
    SocketSendFunc send;
    void* context;
} Server;

typedef struct {
    int socket;
    char address[INET6_ADDRSTRLEN];
    Server* server;
    Allocator* allocator;
    Buffer* buf_in;
    void* context;
} ServerConnection;

Server* serverNew(Allocator* allocator, Sockets* sockets, char* port);
bool serverInit(Server* server, Allocator* allocator, Sockets* sockets, char* port);
void serverClose(Server* server);

#endif