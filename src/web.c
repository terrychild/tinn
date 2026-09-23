#include <stdlib.h>

#include "lib/log.h"
#include "lib/cli.h"
#include "lib/mem/allocator.h"
#include "lib/net/sockets.h"
#include "lib/net/server.h"
#include "version.h"

int hostWebServer(int argc, char* argv[]) {
    logOpen("./tinn.log");
    LOG("Tinn Web Server %s (%s)", VERSION, BUILD_DATE);

    // create server
    Allocator* allocator = allocatorNew();
    Sockets* sockets = socketsNew(allocator, 0);
    Server* server = serverNew(allocator, sockets, cliValue(argc, argv, "--port", "8080"));
    if (server == NULL) {
        ERROR("creating web server");
        return EXIT_FAILURE;
    }

    // loop forever directing network traffic
    LOG("Waiting for connections");
    socketsPoll(sockets);

    // tidy up, but we should never get here?
    DEBUG("Tidying up");
    allocatorRelease(allocator);
    DEBUG("Tidy up complete");
    logClose();

    return EXIT_SUCCESS;
}