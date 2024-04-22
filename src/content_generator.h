#ifndef TINN_CONTENT_GENERATOR_H
#define TINN_CONTENT_GENERATOR_H

#include <stdbool.h>
#include "request.h"
#include "response.h"

typedef bool (*content_generator)(Request* request, Response* response);

typedef struct {
	int count;
	content_generator* generators;
} ContentGenerators;

ContentGenerators* content_generators_new(int count);
void content_generators_free(ContentGenerators* content);

#endif