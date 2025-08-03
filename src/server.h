#ifndef TINN_SERVER_H
#define TINN_SERVER_H

#include "content_generator.h"
#include "net.h"

typedef struct {
	ContentGenerators* content;
	bool tls;
} ServerState;

void server_new(Sockets* sockets, int socket, bool tls, ContentGenerators* content);

#endif