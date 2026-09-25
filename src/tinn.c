#include <stdlib.h>
#include <string.h>

#include "lib/cli.h"
#include "lib/log.h"
#include "version.h"
#include "help.h"
#include "test/tests.h"
#include "lib/mem/allocator.h"
#include "lib/net/echo.h"
#include "web.h"

int host(int argc, char* argv[]) {
    logOpen("./tinn.log");
    LOG("Tinn Web Server %s (%s)", VERSION, BUILD_DATE);

    // resources
    Allocator* allocator = allocatorNew();
    Polling* polling = pollingNew(allocator, 0);

    // servers
    if (cliArg(argc, argv, "--echo")) {
        if (echoServer(allocator, polling, cliValue(argc, argv, "--echo", "7")) == NULL) {
            return EXIT_FAILURE;
        }
    }
    if (startWebServer(allocator, polling, cliValue(argc, argv, "--port", "80")) == NULL) {
        return EXIT_FAILURE;
    }

    // loop while there are things to poll
    LOG("Waiting for connections");
    pollingPoll(polling);

    // tidy up
    DEBUG("Tidying up");
    allocatorRelease(allocator);
    logClose();

    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    if (cliArg(argc, argv, "--verbose")) {
        logLevel = LL_DEBUG;
    }

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

    if (strcmp(argv[1], "host")==0) {
        return host(argc, argv);
    }

    PRINT(CC_BRIGHT_RED, "Error: ");
    PRINT(CC_WHITE, "Unknown command ");
    PRINT(CC_CYAN, "%s\n", argv[1]);
    return EXIT_FAILURE;
}