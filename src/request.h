#ifndef TINN_REQUEST_H
#define TINN_REQUEST_H

#include <stdbool.h>
#include "buffer.h"
#include "scanner.h"
#include "uri.h"
#include <time.h>
#include <sys/types.h>

typedef struct {
	bool complete;

	Buffer* buf;
	int content_start;

	Token method;
	URI* target;
	Token version;

	time_t if_modified_since;
} Request;

Request* request_new();
void request_free(Request* request);

void request_reset(Request* request);

ssize_t request_recv(Request* request, int socket);

#endif