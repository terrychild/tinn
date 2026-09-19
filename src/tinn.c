#include <stdlib.h>
#include <string.h>

#include "lib/cli.h"
#include "help.h"
#include "test/tests.h"

int main(int argc, char* argv[]) {
    if (argc<2) {
        printUsage();  
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "version")==0) {
        printVersion();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "help")==0) {
        return printHelp(argc, argv);
    }

    if (strcmp(argv[1], "test")==0) {
        return runTests();
    }

    PRINTC(CC_BRIGHT_RED, "Error: ");
    PRINTC(CC_WHITE, "Unknown command ");
    PRINTC(CC_CYAN, "%s\n", argv[1]);
    return EXIT_FAILURE;
}