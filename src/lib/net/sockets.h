#ifndef LIB_NET_SOCKETS_H
#define LIB_NET_SOCKETS_H

#include <poll.h>

#include "lib/types.h"

typedef void (*SocketCallbackFunc)(struct pollfd* pfd, void* context, bool* close);

typedef struct {
    SocketCallbackFunc func;
    void* context;
} SocketCallback;

typedef struct {
    Array* pollfds;
    Array* callbacks;
} Sockets;

Sockets* socketsNew(Allocator* allocator, U64 max_capacity);
void socketsInit(Sockets* list, Allocator* allocator, U64 max_capacity);

struct pollfd* socketsAdd(Sockets* list, int new_socket, SocketCallback callback);
void socketsRemove(Sockets* list, int old_socket);

void socketsPoll(Sockets* list);

#endif