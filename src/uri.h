#ifndef TINN_URI_H
#define TINN_URI_H

#include <stdbool.h>
#include "scanner.h"

typedef struct {
	bool valid;
	char* data;
	char* path;
	char* query;
	char** segments;
	int segments_count;
} URI;

URI* uri_new(Token token);
void uri_free(URI* uri);

#endif