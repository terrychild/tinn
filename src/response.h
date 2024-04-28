#ifndef TINN_RESPONSE_H
#define TINN_RESPONSE_H

#include "buffer.h"
#include <time.h>

typedef struct {
	int status_code;

	size_t headers_size;
	size_t headers_count;
	char** header_names;
	char** header_values;

	const char* type;
	Buffer* content;

	bool built;
	Buffer* buf;
} Response;

Response* response_new();
void response_free(Response* response);

void response_reset(Response* response);

void response_status(Response* response, int status_code);
void response_header(Response* response, const char* name, const char* value);
void response_date(Response* response, const char* name, time_t date);
Buffer* response_content(Response* response, char* type);

Buffer* response_buf(Response* response);

void response_error(Response* response, int status_code);
void response_redirect(Response* response, char* location);

#endif