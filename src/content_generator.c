#include "utils.h"
#include "content_generator.h"

ContentGenerators* content_generators_new(int count) {
	ContentGenerators* content = allocate(NULL, sizeof(*content));
	
	content->count = count;
	content->generators = allocate(NULL, sizeof(*content->generators) * count);
	
	return content;
}
void content_generators_free(ContentGenerators* content) {
	if (content != NULL) {
		free(content->generators);
		free(content);
	}
}