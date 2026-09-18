#ifndef LIB_NET_SOCKETS_H
#define LIB_NET_SOCKETS_H

#include <poll.h>

#include "lib/types.h"
#include "lib/mem/array.h"

typedef void (*SocketEventFunc)(struct pollfd* pfd, void* context, bool* close);
typedef void (*SocketCloseFunc)(void* context);

typedef struct {
    SocketEventFunc eventFunc;
    SocketCloseFunc closeFunc; 
    void* context;
} SocketCallback;

typedef struct {
    Array pollfds;
    Array callbacks;
} Sockets;

void socketsInit(Sockets* list, ArenaPool* arena_pool, U64 max_capacity);
void socketsRelease(Sockets* list);
void socketsAdd(Sockets* list, int new_socket, SocketCallback callback);
void socketsPoll(Sockets* list);

#endif