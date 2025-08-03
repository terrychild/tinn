#include <stdlib.h>
#include <string.h>

#include "args.h"

int arg(int argc, char* argv[], const char* name) {
	size_t len = strlen(name);
	for (int i=0; i<argc; i++) {
		if (strncmp(argv[i], name, len)==0) {
			return i;
		}
	}
	return -1;
}