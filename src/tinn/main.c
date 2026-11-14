#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/cli.h"
#include "lib/log.h"
#include "lib/mem/memory.h"
#include "lib/net/echo.h"
#include "lib/net/http.h"

#include "version.h"
#include "help.h"
#include "static.h"
#include "blog.h"

//#include "tls.h"

int host(int argc, char* argv[]) {
    logOpen("./tinn.log");
    LOG("Tinn Web Server %s (%s)", VERSION, BUILD_DATE);

    // change working directory to content directory
    char* content_dir = cliValue(argc, argv, "--dir", ".");
    if (chdir(content_dir) != 0) {
        ERROR("Invalid content directory (%s)", content_dir);
        return EXIT_FAILURE;
    }
    DEBUG("Serving content from: %s", content_dir);

    // resources
    Memory* memory = memoryNew(0);
    Polling* polling = pollingNew(memory, 32);
    Blog* blog = blogNew(memory, polling);
    if (!blog) {
        return EXIT_FAILURE;
    }

    // servers
    if (cliArg(argc, argv, "--echo")) {
        if (!echoServer(serverNew(memory, polling, cliValue(argc, argv, "--echo", "7")))) {
            return EXIT_FAILURE;
        }
    }
    HttpServer* http_server = httpServer(memory, serverNew(memory, polling, cliValue(argc, argv, "--port", "80")));
    if (http_server) {
        httpServerAddListener(http_server, staticContent, NULL);
        httpServerAddListener(http_server, blogContent, blog);
    } else {
        return EXIT_FAILURE;
    }

    ////tlsTestServer(memory, polling, "8443");

    // loop while there are things to poll
    LOG("Waiting for connections");
    pollingPoll(polling);

    // tidy up
    DEBUG("Tidying up");
    memoryRelease(memory);
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

    if (strcmp(argv[1], "host")==0) {
        return host(argc, argv);
    }

    PRINT(CC_BRIGHT_RED, "Error: ");
    PRINT(CC_WHITE, "Unknown command ");
    PRINT(CC_CYAN, "%s\n", argv[1]);
    return EXIT_FAILURE;
}