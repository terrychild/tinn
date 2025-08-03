#ifndef TINN_CONSOLE_H
#define TINN_CONSOLE_H

#include <stdlib.h>
#include <stdio.h>

typedef enum {
	CC_BLACK,
	CC_RED,
	CC_GREEN,
	CC_YELLOW,
	CC_BLUE,
	CC_MAGENTA,
	CC_CYAN,
	CC_WHITE,

	CC_BRIGHT_BLACK,
	CC_BRIGHT_RED,
	CC_BRIGHT_GREEN,
	CC_BRIGHT_YELLOW,
	CC_BRIGHT_BLUE,
	CC_BRIGHT_MAGENTA,
	CC_BRIGHT_CYAN,
	CC_BRIGHT_WHITE,
	
	CC_BOLD_BLACK,
	CC_BOLD_RED,
	CC_BOLD_GREEN,
	CC_BOLD_YELLOW,
	CC_BOLD_BLUE,
	CC_BOLD_MAGENTA,
	CC_BOLD_CYAN,
	CC_BOLD_WHITE
} ConsoleColour;

typedef enum {
	CL_TRACE =	0,
	CL_DEBUG =	1,
	CL_INFO =	2,
	CL_WARN =	3,
	CL_ERROR = 	4,
	CL_PANIC =	5
} ConsoleLevel;

extern ConsoleLevel clevel;

void print(FILE* stream, ConsoleColour colour, const char* format, ...);
void console(FILE* stream, ConsoleLevel level, bool inc_time, bool inc_errno, const char* format, ...);

#define PRINT(colour, ...) print(stdout, colour, __VA_ARGS__);

#define TRACE(...) console(stdout, CL_TRACE, true, false, __VA_ARGS__)
#define TRACE_DETAIL(...) console(stdout, CL_TRACE, false, false, __VA_ARGS__)
#define DEBUG(...) console(stdout, CL_DEBUG, true, false, __VA_ARGS__)
#define DEBUG_DETAIL(...) console(stdout, CL_DEBUG, false, false, __VA_ARGS__)
#define LOG(...) console(stdout, CL_INFO, true, false, __VA_ARGS__)
#define LOG_DETAIL(...) console(stdout, CL_INFO, false, false, __VA_ARGS__)
#define WARN(...) console(stdout, CL_WARN, true, false, __VA_ARGS__)
#define ERROR(...) console(stderr, CL_ERROR, true, true, __VA_ARGS__)
#define PANIC(...) console(stderr, CL_PANIC, true, true, __VA_ARGS__); exit(EXIT_FAILURE)

#endif