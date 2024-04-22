#ifndef TINN_SERVER_H
#define TINN_SERVER_H

#include "content_generator.h"
#include "net.h"

typedef struct {
	ContentGenerators* content;
} ServerState;

void server_new(Sockets* sockets, int socket, ContentGenerators* content);
//void server_listener(Sockets* sockets, int index);

#endif