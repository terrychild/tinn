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

void logAppend(LogLevel level, bool inc_errno, const char* format, ...);

#define DEBUG(...) logAppend(LL_DEBUG, false, __VA_ARGS__)
#define LOG(...) logAppend(LL_INFO, false, __VA_ARGS__)
#define WARN(...) logAppend(LL_WARN, false, __VA_ARGS__)
#define ERROR(...) logAppend(LL_ERROR, true, __VA_ARGS__)
#define PANIC(...) logAppend(LL_PANIC, true, __VA_ARGS__); exit(EXIT_FAILURE)

void logOpen(const char* path);
void logClose();

#endif