#ifndef TINN_CONSOLE_H
#define TINN_CONSOLE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum {
	CL_TRACE =	0,
	CL_LOG =	1,
	CL_WARN =	2,
	CL_ERROR = 	3,
	CL_PANIC =	4
} ConsoleLevel;

extern ConsoleLevel console_level;

void console(FILE *stream, ConsoleLevel level, bool inc_time, bool inc_errno, const char* format, ...);

#define TRACE(...) console(stdout, CL_TRACE, true, false, __VA_ARGS__)
#define TRACE_DETAIL(...) console(stdout, CL_TRACE, false, false, __VA_ARGS__)
#define LOG(...) console(stdout, CL_LOG, true, false, __VA_ARGS__)
#define LOG_DETAIL(...) console(stdout, CL_LOG, false, false, __VA_ARGS__)
#define WARN(...) console(stdout, CL_WARN, true, true, __VA_ARGS__)
#define ERROR(...) console(stderr, CL_ERROR, true, true, __VA_ARGS__)
#define PANIC(...) console(stderr, CL_PANIC, true, true, __VA_ARGS__); exit(EXIT_FAILURE)

#endif