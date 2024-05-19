#ifndef TINN_RESPONSE_H
#define TINN_RESPONSE_H

#include "buffer.h"
#include <time.h>

#define RESPONSE_PREP 0
#define RESPONSE_HEADERS 1
#define RESPONSE_CONTENT 2
#define RESPONSE_DONE 3

typedef struct {
	int status_code;

	size_t headers_size;
	size_t headers_count;
	char** header_names;
	char** header_values;

	unsigned short content_source;
	const char* type;
	Buffer* content;
	size_t content_length;

	Buffer* headers;
	unsigned short stage;
} Response;

Response* response_new();
void response_reset(Response* response);
void response_free(Response* response);

void response_status(Response* response, int status_code);
void response_header(Response* response, const char* name, const char* value);
void response_date(Response* response, const char* name, time_t date);

void repsonse_no_content(Response* response);
void repsonse_content_headers(Response* response, char* type, size_t length);
Buffer* response_content(Response* response, char* type);
void repsonse_link_content(Response* response, Buffer* buf, char* type);

ssize_t response_send(Response* response, int socket);

void response_error(Response* response, int status_code);
void response_redirect(Response* response, char* location);

#endif