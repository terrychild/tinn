#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/mem/arena.h"
#include "lib/net/sockets.h"

typedef void* (*SocketOpenFunc)(void* context);
typedef void (*SocketReceiveFunc)(void* context);
typedef void (*SocketSendFunc)(void* context);

typedef struct {
    Arena* arena;
    Sockets* sockets;
    //TODO:Pool connections;
    SocketOpenFunc openConnection;
    SocketCloseFunc closeConnection;
    SocketReceiveFunc receive;
    SocketSendFunc send;
    void* context;
} Server;

typedef struct {
    Server* server;
    //TODO:Arena* arena;
    void* context;
    int socket;
    char address[INET6_ADDRSTRLEN];
} ServerConnection;

Server* serverNew(Arena* arena, Sockets* sockets, char* port);
bool serverInit(Server* server, Arena* arena, Sockets* sockets, char* port);

#endif