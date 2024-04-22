#ifndef TINN_REQUEST_H
#define TINN_REQUEST_H

#include "buffer.h"
#include <time.h>

typedef struct {
	Buffer* buf;
	time_t h_if_modified_Since;
} Request;

Request* request_new();
void request_free(Request* request);

void request_reset_headers(Request* request);

#endif