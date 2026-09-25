#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "lib/macros.h"
#include "lib/log.h"
#include "lib/cli.h"

static constexpr U64 MESSAGE_LEN = KB(4);
static const char* logLevels[] = {
    [LL_DEBUG] = "DEBUG",
    [LL_INFO]  = "INFO", 
    [LL_WARN]  = "WARN",
    [LL_ERROR] = "ERROR",
    [LL_PANIC] = "PANIC"
};

static FILE* file = NULL;

LogLevel logLevel = LL_INFO;

static void formatMessage(char* message, bool inc_errno, const char* format, va_list args) {
    U64 len = vsnprintf(message, MESSAGE_LEN, format, args);

    if (inc_errno && errno != 0) {
        if (len < MESSAGE_LEN) {
            snprintf(message + len, MESSAGE_LEN - len, " -> %s", strerror(errno));
        }
    }
}

static void colourPrint(FILE* stream, time_t* now, LogLevel level, char* message) {
    char timestamp[9];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(now));
    print(stream, CC_BLUE, "%s ", timestamp);
    
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

static void filePrint(FILE* stream, time_t* now, LogLevel level, char* message) {
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(now));

    fprintf(stream, "%s [%s] %s\n",
        timestamp,
        logLevels[level],        
        message
    );
}

void logAppend(LogLevel level, bool inc_errno, const char* format, ...) {
    if (level >= logLevel) {
        time_t now = time(NULL);

        char message[MESSAGE_LEN];
        va_list args;
        va_start(args, format);
        formatMessage(message, inc_errno, format, args);
        va_end(args);

        // cli
        colourPrint(stdout, &now, level, message);

        // file
        if (file != NULL) {
            filePrint(file, &now, level, message);
            fflush(file);   
        }
    }
}

void logOpen(const char* path) {
    logClose();

    file = fopen(path, "w");
    if (file == NULL) {
        time_t now = time(NULL);
        colourPrint(stdout, &now, LL_PANIC, "Unable to log to file");
        exit(EXIT_FAILURE);
    }
}
void logClose() {
    if (file != NULL) {
        fflush(file);
        fclose(file);
        file = NULL;
    }
}