#ifndef TINN_NET_H
#define TINN_NET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>

#include "routes.h"

typedef struct sockets_list Sockets;
typedef void (*socket_listener)(Sockets* sockets, int index, Routes* routes);

struct sockets_list {
	size_t  size;
	size_t  count;
	struct pollfd* pollfds;
	socket_listener* listeners;
	void** states;
};

int get_server_socket(char* port);

Sockets* sockets_new();
int sockets_add(Sockets* list, int new_socket, socket_listener new_listener);
void sockets_rm(Sockets* list, size_t index);

#endif