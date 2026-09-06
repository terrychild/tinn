#ifndef LIB_NET_SOCKETS_H
#define LIB_NET_SOCKETS_H

#include <poll.h>

#include "lib/types.h"

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

Sockets* socketsNew();
void socketsRelease(Sockets* list);
void socketsAdd(Sockets* list, int new_socket, SocketCallback callback);
void socketsPoll(Sockets* list);

#endif