#ifndef TINN_RESPONSE_H
#define TINN_RESPONSE_H

#include "buffer.h"

typedef struct {
	Buffer* buf;
} Response;

Response* response_new();
void response_free(Response* response);

#endif