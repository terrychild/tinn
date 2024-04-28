#ifndef TINN_CONTENT_GENERATOR_H
#define TINN_CONTENT_GENERATOR_H

#include <stdbool.h>
#include "request.h"
#include "response.h"

typedef bool (*content_generator)(void* state, Request* request, Response* response);

typedef struct {
	size_t size;
	size_t count;
	content_generator* generators;
	void** states;
} ContentGenerators;

ContentGenerators* content_generators_new(size_t size);
void content_generators_free(ContentGenerators* content);

void content_generators_add(ContentGenerators* content, content_generator generator, void* state);

#endif