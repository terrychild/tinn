#ifndef TINN_CLIENT_H
#define TINN_CLIENT_H

#include "utils.h"
#include "content_generator.h"
#include "net.h"
#include "request.h"
#include "response.h"

#define CLIENT_READ 1;
#define CLIENT_WRITE 2;

typedef struct {
	ContentGenerators* content;
	char address[INET6_ADDRSTRLEN];
	unsigned short mode;
	Request* request;
	Response* response;
} ClientState;

ClientState* client_state_new();
void client_state_free(ClientState* state);

void client_listener(Sockets* sockets, int index);

#endif