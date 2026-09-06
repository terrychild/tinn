#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/net/sockets.h"

typedef void* (*SocketConnectFunction)(void* context);
typedef void (*SocketReceiveFunc)(void* context);
typedef void (*SocketSendFunc)(void* context);

typedef struct {
    Sockets* sockets;
    SocketConnectFunction connect;
    SocketReceiveFunc receive;
    SocketSendFunc send;
    SocketCloseFunc close;
    void* context;    
} Server;

typedef struct {
    char address[INET6_ADDRSTRLEN];
    SocketReceiveFunc receive;
    SocketSendFunc send;
    SocketCloseFunc close;
    void* context;
} ServerConnection;

Server* serverNew(Sockets* list, char* port);
//TODO: void serverClose(Server* server)

#endif