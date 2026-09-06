#include "utils.h"
#include "content_generator.h"

ContentGenerators* content_generators_new(size_t size) {
	ContentGenerators* content = allocate(NULL, sizeof(*content));
	
	content->size = size;
	content->count = 0;
	content->generators = allocate(NULL, sizeof(*content->generators) * size);
	content->states = allocate(NULL, sizeof(*content->states) * size);
	
	return content;
}
void content_generators_free(ContentGenerators* content) {
	if (content != NULL) {
		free(content->generators);
		free(content->states);
		free(content);
	}
}

void content_generators_add(ContentGenerators* content, content_generator generator, void* state) {
	if (content->count == content->size) {
		content->size *= 2;
		content->generators = allocate(content->generators, sizeof(*content->generators) * content->size);
		content->states = allocate(content->states, sizeof(*content->states) * content->size);
	}

	content->generators[content->count] = generator;
	content->states[content->count] = state;

	content->count++;
}