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
    Array* pollfds;
    Array* callbacks;
} Sockets;

Sockets* socketsNew(Allocator* allocator, U64 max_capacity);
void socketsInit(Sockets* list, Allocator* allocator, U64 max_capacity);

void socketsAdd(Sockets* list, int new_socket, SocketCallback callback);
void socketsRemove(Sockets* list, int old_socket);

void socketsPoll(Sockets* list);

#endif