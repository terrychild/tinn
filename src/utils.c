#include "utils.h"
#include "console.h"

void* allocate(void* data, size_t size) {
	void* new_data = realloc(data, size);
	if (new_data == NULL) {
		PANIC("unable to allocate memory");
	}
	return new_data;
}