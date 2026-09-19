#ifndef LIB_LOG_H
#define LIB_LOG_H

#include <stdlib.h>

typedef enum {
    LL_DEBUG =  0,
    LL_INFO =   1,
    LL_WARN =   2,
    LL_ERROR =  3,
    LL_PANIC =  4
} LogLevel;

extern LogLevel logLevel;

void appendToLog(LogLevel level, bool inc_errno, const char* format, ...);

#define DEBUG(...) appendToLog(LL_DEBUG, false, __VA_ARGS__)
#define LOG(...) appendToLog(LL_INFO, false, __VA_ARGS__)
#define WARN(...) appendToLog(LL_WARN, false, __VA_ARGS__)
#define ERROR(...) appendToLog(LL_ERROR, true, __VA_ARGS__)
#define PANIC(...) appendToLog(LL_PANIC, true, __VA_ARGS__); exit(EXIT_FAILURE)

#endif