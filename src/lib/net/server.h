#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/types.h"
#include "lib/net/sockets.h"

typedef void* (*SocketOpenFunc)(void* context);
typedef void (*SocketReceiveFunc)(void* context);
typedef void (*SocketSendFunc)(void* context);

typedef struct {
    ArenaPool* arena_pool;
    Sockets* sockets;
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
    char address[INET6_ADDRSTRLEN];
} ServerConnection;

bool serverInit(Server* server, ArenaPool* arena_pool, Sockets* sockets, char* port);
//TODO: void serverClose(Server* server)

#endif