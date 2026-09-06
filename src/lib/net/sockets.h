#ifndef LIB_NET_SOCKETS_H
#define LIB_NET_SOCKETS_H

#include <poll.h>
#include <netinet/in.h>

#include "lib/types.h"

// Generic socket stuff
int getServerSocket(char* port);

// Socket list managment

typedef void (*SocketEventFunc)(struct pollfd* pfd, void* context, bool* close);
typedef void (*SocketCloseFunc)(void* context);

typedef struct {
    SocketEventFunc eventFunc;
    SocketCloseFunc closeFunc; 
    void* context;
} SocketCallback;

typedef struct {
    U64 size;
    U64 count;
    struct pollfd* pollfds;
    SocketCallback* callbacks;
} Sockets;

void socketsInit(Sockets* list);
void socketsFree(Sockets* list);
void socketsAdd(Sockets* list, int new_socket, SocketCallback callback);
void socketsPoll(Sockets* list);

#endif