#include <stdlib.h>

#include "lib.h"
#include "version.h"

int hostWebServer(int argc, char* argv[]) {
    LOG("Tinn Web Server %s (%s)", VERSION, BUILD_DATE);
    
    // create list of sockets
    DEBUG("Creating sockets list");
    Sockets sockets;
    socketsInit(&sockets);

    // create server
    DEBUG("Creating Web server");
    SocketServer* server = socketServerNew(&sockets, cliValue(argc, argv, "--port", "8080"));
    if (server == NULL) {
        ERROR("creating web server");
        return EXIT_FAILURE;
    }

    // loop forever directing network traffic
    LOG("Waiting for connections");
    socketsPoll(&sockets);

    // tidy up, but we should never get here?
    DEBUG("Tidying up");
    //content_generators_free(content);
    socketsFree(&sockets);
    DEBUG("Tidy up complete");
    
    return EXIT_SUCCESS;
}