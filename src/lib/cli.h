#ifndef LIB_CLI_H
#define LIB_CLI_H

#include <stdlib.h>
#include <stdio.h>

typedef enum {
    CC_NULL = 0,
    CC_RESET,

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
} ColourCode;

void printColour(FILE* stream, ColourCode colour);
void print(FILE* stream, ColourCode colour, const char* format, ...);

#define PRINT(colour, ...) print(stdout, colour, __VA_ARGS__);

bool cliArg(int argc, char* argv[], const char* name);

#endif