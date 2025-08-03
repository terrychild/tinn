#ifndef TINN_SESSION_H
#define TINN_SESSION_H

#include "content_generator.h"
#include "net.h"
#include "buffer.h"

typedef enum {
	STATE_START,
	// STATE_RECVD_CH,
	// STATE_NEGOTIATED,
	// STATE_WAIT_EOED,
	// STATE_WAIT_FLIGHT2,
	// STATE_WAIT_CERT,
	// STATE_WAIT_CV,
	// STATE_WAIT_FINISHED,
	STATE_CONNECTED
} State;

typedef struct {
	ContentGenerators* content;
	bool tls;
	char address[INET6_ADDRSTRLEN];

	Buffer* bufin;
	Buffer* bufout;

	State state;
	bool close;
} Session;

Session* session_new();
void session_free(Session* session);

void session_listener(Sockets* sockets, int index);

#endif