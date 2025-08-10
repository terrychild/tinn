#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "lib/console.h"

ConsoleLevel clevel = CL_DEBUG;

static void set_colour(FILE* stream, ConsoleColour colour) {
    switch(colour) {
        case CC_BLACK: fputs("\x1B[30m", stream); break;
        case CC_RED: fputs("\x1B[31m", stream); break;
        case CC_GREEN: fputs("\x1B[32m", stream); break;
        case CC_YELLOW: fputs("\x1B[33m", stream); break;
        case CC_BLUE: fputs("\x1B[34m", stream); break;
        case CC_MAGENTA: fputs("\x1B[35m", stream); break;
        case CC_CYAN: fputs("\x1B[36m", stream); break;
        case CC_WHITE: fputs("\x1B[37m", stream); break;

        case CC_BRIGHT_BLACK: fputs("\x1B[90m", stream); break;
        case CC_BRIGHT_RED: fputs("\x1B[91m", stream); break;
        case CC_BRIGHT_GREEN: fputs("\x1B[92m", stream); break;
        case CC_BRIGHT_YELLOW: fputs("\x1B[93m", stream); break;
        case CC_BRIGHT_BLUE: fputs("\x1B[94m", stream); break;
        case CC_BRIGHT_MAGENTA: fputs("\x1B[95m", stream); break;
        case CC_BRIGHT_CYAN: fputs("\x1B[96m", stream); break;
        case CC_BRIGHT_WHITE: fputs("\x1B[97m", stream); break;

        case CC_BOLD_BLACK: fputs("\x1B[1;90m", stream); break;
        case CC_BOLD_RED: fputs("\x1B[1;91m", stream); break;
        case CC_BOLD_GREEN: fputs("\x1B[1;92m", stream); break;
        case CC_BOLD_YELLOW: fputs("\x1B[1;93m", stream); break;
        case CC_BOLD_BLUE: fputs("\x1B[1;94m", stream); break;
        case CC_BOLD_MAGENTA: fputs("\x1B[1;95m", stream); break;
        case CC_BOLD_CYAN: fputs("\x1B[1;96m", stream); break;
        case CC_BOLD_WHITE: fputs("\x1B[1;97m", stream); break;
    }
}
static void reset_colour(FILE* stream) {
    fputs("\x1B[0m", stream);
}

void print(FILE *stream, ConsoleColour colour, const char* format, ...) {
    set_colour(stream, colour);
        
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);

    reset_colour(stream);
}

static void print_time(FILE* stream) {
    time_t seconds = time(NULL);
    struct tm* gmt = gmtime(&seconds);
    print(stream, CC_BLUE, "%02d:%02d:%02d ", gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
}

static void print_prefix(FILE* stream, ConsoleLevel level) {
    switch(level) {
        case CL_DEBUG:
            set_colour(stream, CC_CYAN);
            break;
        case CL_INFO:
            break;
        case CL_WARN:
            print(stream, CC_YELLOW, "warning: ");
            break;
        case CL_ERROR:
            print(stream, CC_BRIGHT_RED, "error: ");
            break;
        case CL_PANIC:
            print(stream, CC_BOLD_RED, "PANIC: ");
            break;
    }
}
static void print_postfix(FILE* stream, ConsoleLevel level) {
    switch(level) {
        case CL_DEBUG:
        case CL_WARN:
        case CL_ERROR:
        case CL_PANIC:
            reset_colour(stream);
            break;
        case CL_INFO:
            break;
    }
}

void console(FILE* stream, ConsoleLevel level, bool inc_time, bool inc_errno, const char* format, ...) {
    if (level >= clevel) {
        if (inc_time) {
            print_time(stream);
        }
        print_prefix(stream, level);
        
        va_list args;
        va_start(args, format);
        vfprintf(stream, format, args);
        va_end(args);

        if (inc_errno && errno != 0) {
            fprintf(stream, " -> %s", strerror(errno));
        }
        print_postfix(stream, level);
        fputs("\n", stream);
    }
}