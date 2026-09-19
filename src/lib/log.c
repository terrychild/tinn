#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "lib/macros.h"
#include "lib/log.h"
#include "lib/cli.h"

#define MESSAGE_LEN KB(4)

LogLevel logLevel = LL_INFO;

static void formatMessage(char* message, bool inc_errno, const char* format, ...) {
    va_list args;
    va_start(args, format);
    U64 len = snprintf(message, MESSAGE_LEN, format, args);
    va_end(args);

    if (inc_errno && errno != 0) {
        if (len < MESSAGE_LEN) {
            snprintf(message + len, MESSAGE_LEN - len, " -> %s", strerror(errno));
        }
    }
}

static void colourPrint(FILE* stream, LogLevel level, struct tm* gmt, char* message) {
    print(stream, CC_BLUE, "%02d:%02d:%02d ", gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
    
    switch(level) {
        case LL_DEBUG:
            printColour(stream, CC_CYAN);
        case LL_INFO:
            break;
        case LL_WARN:
            print(stream, CC_YELLOW, "Warning: ");
            break;
        case LL_ERROR:
            print(stream, CC_BRIGHT_RED, "Error: ");
            break;
        case LL_PANIC:
            print(stream, CC_BOLD_RED, "PANIC: ");
            break;
    }

    print(stream, CC_NULL, message);
    print(stream, CC_RESET, "\n");
}

void appendToLog(LogLevel level, bool inc_errno, const char* format, ...) {
    if (level >= logLevel) {
        time_t seconds = time(NULL);
        struct tm gmt;
        gmtime_r(&seconds, &gmt);

        char message[MESSAGE_LEN];
        va_list args;
        va_start(args, format);
        formatMessage(message, inc_errno, format, args);
        va_end(args);

        // cli
        colourPrint(stdout, level, &gmt, message);
    }
}