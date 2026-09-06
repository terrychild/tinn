#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "version.h"
#include "help.h"
#include "test/tests.h"
#include "web.h"

static void printVersion() {
    PRINT(CC_BRIGHT_WHITE, "Tinn %s ", VERSION);
    PRINT(CC_BRIGHT_BLACK, "(%s)\n", BUILD_DATE);
}

int main(int argc, char* argv[]) {
    if (cliArg(argc, argv, "--verbose")) {
        clevel = CL_DEBUG;
    }

    if (argc<2) {
        printUsage();  
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "help")==0) {
        return printHelp(argc, argv);
    }

    if (strcmp(argv[1], "test")==0) {
        return runTests();
    }

    if (strcmp(argv[1], "version")==0) {
        printVersion();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "host")==0) {
        return hostWebServer(argc, argv);
    }

    PRINT(CC_BRIGHT_RED, "Error: ");
    PRINT(CC_WHITE, "Unknown command ");
    PRINT(CC_CYAN, "%s\n", argv[1]);
    return EXIT_FAILURE;
}