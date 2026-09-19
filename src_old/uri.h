#ifndef TINN_URI_H
#define TINN_URI_H

#include <stdbool.h>
#include "scanner.h"

typedef struct {
	bool valid;
	char* data;
	char* path;
	size_t path_len;
	char* query;
	size_t query_len;
	char** segments;
	size_t segments_count;
} URI;

URI* uri_new(Token token);
void uri_free(URI* uri);

#endif