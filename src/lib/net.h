#ifndef NET_H
#define NET_H

#include <poll.h>

#include "lib/types.h"

typedef struct socket Socket;
typedef void (*SocketListener)(Socket socket);

struct socket {
    struct pollfd pollfd;
    SocketListener listener;
    void* state;
};

typedef struct {
    U64  size;
    U64  count;
    struct socket* sockets;
} Sockets;

int getLocalSocket(char* port);

void socketsInit(Sockets* list);
void socketsFree(Sockets* list);
void socketsAdd(Sockets* list, int new_socket, SocketListener new_listener, void* new_state);
void socketsRm(Sockets* list, U64 index);

#endif