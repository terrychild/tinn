#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "help.h"
#include "console.h"

static void printCommand(const char* args) {
    PRINT(CC_BLUE, "tinn ");
    PRINT(CC_CYAN, args);
}
static void printArg(const char* arg, const char* descripton) {
    PRINT(CC_CYAN, "  %-17s ", arg);
    PRINT(CC_WHITE, "%s\n", descripton);
}
static void printArgCont(const char* descripton) {
    PRINT(CC_WHITE, "                    %s\n", descripton);
}

void printUsage() {
    PRINT(CC_WHITE, "Usage: ");
    printCommand("<command> [<args>]\n\n");
    PRINT(CC_GREEN, "Commands\n");
    printArg("host", "Host a tinn web server.");
    printArg("help", "Display help.");
    printArg("version", "Display the current version and build date.");
    printArg("test", "Run test suite.");
    PRINT(CC_WHITE, "\nFor help on a specific command see: ");
    printCommand("help <command>\n");
}

static void hostHelp() {
    printCommand("tinn host [<args>]\n\n");
    PRINT(CC_WHITE, "Host a Tinn web server.\n\n");
    PRINT(CC_GREEN, "Arguments\n");
    printArg("--dir=<path>", "Path for the content directoy, defaults to current");
    printArgCont("directory.");
    printArg("--port=<num>", "Port to listen on, defaults to 8080.");
    printArg("--verbose", "Enable verbose logging.");    
}

static void testHelp() {
    printCommand("test\n\n");
    puts("Runs a series of tests to ensure Tinn is (possibly) working.  It's hard to");
    puts("prove these things, but it will certainly flag any obvious errors.");
}

int printHelp(int argc, char* argv[]) {
    if (argc<3) {
        printUsage();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[2], "host")==0) {
        hostHelp();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[2], "test")==0) {
        testHelp();
        return EXIT_SUCCESS;
    }

    printf("No help available for command \"%s\".\n", argv[2]);
    return EXIT_FAILURE;
}