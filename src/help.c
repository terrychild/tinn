#include <string.h>

#include "lib/cli.h"
#include "version.h"

void printVersion() {
    PRINTC(CC_BRIGHT_WHITE, "Tinn %s ", VERSION);
    PRINTC(CC_BRIGHT_BLACK, "(%s)\n", BUILD_DATE);
}

static void printCommand(const char* args) {
    PRINTC(CC_BLUE, "tinn ");
    PRINTC(CC_CYAN, args);
}
static void printArg(const char* arg, const char* descripton) {
    PRINTC(CC_CYAN, "  %-17s ", arg);
    PRINTC(CC_WHITE, "%s\n", descripton);
}
static void printArgCont(const char* descripton) {
    PRINTC(CC_WHITE, "                    %s\n", descripton);
}

void printUsage() {
    PRINTC(CC_WHITE, "Usage: ");
    printCommand("<command> [<args>]\n\n");
    PRINTC(CC_GREEN, "Commands\n");
    printArg("host", "Host a tinn web server.");
    printArg("help", "Display help.");
    printArg("version", "Display the current version and build date.");
    printArg("test", "Run test suite.");
    PRINTC(CC_WHITE, "\nFor help on a specific command see: ");
    printCommand("help <command>\n");
}

static void hostHelp() {
    printCommand("host [<args>]\n\n");
    PRINTC(CC_WHITE, "Host a Tinn web server.\n\n");
    PRINTC(CC_GREEN, "Arguments\n");
    printArg("--dir=<path>", "Path for the content directoy, defaults to current");
    printArgCont("directory.");
    printArg("--port=<num>", "Port to listen on, defaults to 8080.");
    printArg("--verbose", "Enable verbose logging.");    
}

static void testHelp() {
    printCommand("test\n\n");
    PRINT("Runs a series of tests to ensure Tinn is (possibly) working.  It's hard to");
    PRINT("prove these things, but it will certainly flag any obvious errors.");
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

    PRINTC(CC_WHITE, "No help available for command ");
    PRINTC(CC_CYAN, "%s\n", argv[2]);
    return EXIT_FAILURE;
}