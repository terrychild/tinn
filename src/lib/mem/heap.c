#include <stdlib.h>

#include "lib/types.h"
#include "lib/console.h"

void* allocate(void* data, U64 size) {
    void* new_data = realloc(data, size);
    if (new_data == NULL) {
        PANIC("Unable to allocate memory");
    }
    return new_data;
}