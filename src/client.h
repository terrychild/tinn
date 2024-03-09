#ifndef TINN_CLIENT_H
#define TINN_CLIENT_H

#include "net.h"

#define CLIENT_READ 1;
#define CLIENT_WRITE 2;

typedef struct {
	char address[INET6_ADDRSTRLEN];
	unsigned short mode;
	Buffer* in;
	Buffer* out;
} ClientState;

ClientState* client_state_new();
void client_state_free(ClientState* state);

void client_listener(Sockets* sockets, int index, Routes* routes);

#endif