#include <stdlib.h>

#include "lib.h"
#include "version.h"

int hostWebServer(int argc, char* argv[]) {
    LOG("Tinn Web Server %s (%s)", VERSION, BUILD_DATE);

    ArenaPool mem;
    arenaPoolInit(&mem, 0, 0);
    
    // create list of sockets
    DEBUG("Creating sockets list");
    Sockets sockets;
    socketsInit(&sockets, 0, &mem);

    // create server
    DEBUG("Creating Web server");
    Server* server = serverNew(&sockets, cliValue(argc, argv, "--port", "8080"));
    if (server == NULL) {
        ERROR("creating web server");
        return EXIT_FAILURE;
    }

    // loop forever directing network traffic
    LOG("Waiting for connections");
    socketsPoll(&sockets);

    // tidy up, but we should never get here?
    DEBUG("Tidying up");
    socketsRelease(&sockets);
    DEBUG("Tidy up complete");
    
    return EXIT_SUCCESS;
}