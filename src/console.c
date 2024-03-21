#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "console.h"

#define RED "\x1B[31m"
#define INTENSE_BOLD_RED "\x1B[1;91m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define CYAN "\x1B[36m"
#define RESET "\x1B[0m"

ConsoleLevel clevel = CL_DEBUG;

static void print_time(FILE *stream) {
	time_t seconds = time(NULL);
	struct tm* gmt = gmtime(&seconds);
	fprintf(stream, BLUE "%02d:%02d:%02d " RESET, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
}

static void print_prefix(FILE *stream, ConsoleLevel level) {
	switch(level) {
		case CL_TRACE:
		case CL_DEBUG:
			fputs(CYAN, stream);
			break;
		case CL_INFO:
			break;
		case CL_WARN:
			fputs(YELLOW "warning: " RESET, stream);
			break;
		case CL_ERROR:
			fputs(RED "error: " RESET, stream);
			break;
		case CL_PANIC:
			fputs(INTENSE_BOLD_RED "PANIC: " RESET, stream);
			break;
	}
}

void console(FILE *stream, ConsoleLevel level, bool inc_time, bool inc_errno, const char* format, ...) {
	if (level >= clevel) {
		if (inc_time) {
			print_time(stream);
		} else {
			fputs("  ", stream);
		}
		print_prefix(stream, level);
		
		va_list args;
		va_start(args, format);
		vfprintf(stream, format, args);
		va_end(args);

		if (inc_errno && errno != 0) {
			fprintf(stream, " -> %s", strerror(errno));
		}
		fputs(RESET "\n", stream);
	}
}

#undef RED
#undef INTENSE_BOLD_RED
#undef YELLOW
#undef BLUE
#undef CYAN
#undef RESET