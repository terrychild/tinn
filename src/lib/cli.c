#include <stdarg.h>
#include <string.h>

#include "lib/cli.h"

static const char* colourEscapeSequences[] = {
    [CC_RESET] = "\x1B[0m",

    [CC_BLACK] =   "\x1B[30m",
    [CC_RED] =     "\x1B[31m",
    [CC_GREEN] =   "\x1B[32m",
    [CC_YELLOW] =  "\x1B[33m",
    [CC_BLUE] =    "\x1B[34m",
    [CC_MAGENTA] = "\x1B[35m",
    [CC_CYAN] =    "\x1B[36m",
    [CC_WHITE] =   "\x1B[37m",

    [CC_BRIGHT_BLACK] =   "\x1B[90m",
    [CC_BRIGHT_RED] =     "\x1B[91m",
    [CC_BRIGHT_GREEN] =   "\x1B[92m",
    [CC_BRIGHT_YELLOW] =  "\x1B[93m",
    [CC_BRIGHT_BLUE] =    "\x1B[94m",
    [CC_BRIGHT_MAGENTA] = "\x1B[95m",
    [CC_BRIGHT_CYAN] =    "\x1B[96m",
    [CC_BRIGHT_WHITE] =   "\x1B[97m",

    [CC_BOLD_BLACK] =   "\x1B[1;90m",
    [CC_BOLD_RED] =     "\x1B[1;91m",
    [CC_BOLD_GREEN] =   "\x1B[1;92m",
    [CC_BOLD_YELLOW] =  "\x1B[1;93m",
    [CC_BOLD_BLUE] =    "\x1B[1;94m",
    [CC_BOLD_MAGENTA] = "\x1B[1;95m",
    [CC_BOLD_CYAN] =    "\x1B[1;96m",
    [CC_BOLD_WHITE] =   "\x1B[1;97m"
};

void printColour(FILE* stream, ColourCode colour) {
    fputs(colourEscapeSequences[colour], stream);
}

void print(FILE *stream, ColourCode colour, const char* format, ...) {
    if (colour != CC_NULL) {
        printColour(stream, colour);
    }
        
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);

    if (colour != CC_NULL) {
        printColour(stream, CC_RESET);
    }
}

bool cliArg(int argc, char* argv[], const char* name) {
    for (int i=0; i<argc; i++) {
        if (strcmp(argv[i], name)==0) {
            return true;
        }
    }
    return false;
}