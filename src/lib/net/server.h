#ifndef LIB_NET_SERVER_H
#define LIB_NET_SERVER_H

#include <netinet/in.h>

#include "lib/types.h"
#include "lib/net/sockets.h"

// Server stuff

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
} SocketServer;

typedef struct {
    char address[INET6_ADDRSTRLEN];
    SocketReceiveFunc receive;
    SocketSendFunc send;
    SocketCloseFunc close;
    void* context;
} SocketServerConnection;

SocketServer* socketServerNew(Sockets* list, char* port);

#endif