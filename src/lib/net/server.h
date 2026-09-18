#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/types.h"
#include "lib/net/sockets.h"
#include "lib/mem/pool.h"

typedef void* (*SocketOpenFunc)(void* context);
typedef void (*SocketReceiveFunc)(void* context);
typedef void (*SocketSendFunc)(void* context);

typedef struct {
    ArenaPool* arena_pool;
    Sockets* sockets;
    Pool connections;
    SocketOpenFunc openConnection;
    SocketCloseFunc closeConnection;
    SocketReceiveFunc receive;
    SocketSendFunc send;
    void* context;
} Server;

typedef struct {
    Server* server;
    Arena* arena;
    void* context;
    int socket;
    char address[INET6_ADDRSTRLEN];
} ServerConnection;

bool serverInit(Server* server, ArenaPool* arena_pool, Sockets* sockets, char* port);
//TODO: void serverClose(Server* server)

#endif