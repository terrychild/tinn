#include <stdlib.h>

#include "lib.h"
#include "version.h"

int hostWebServer(int argc, char* argv[]) {
    LOG("Tinn Web Server %s (%s)", VERSION, BUILD_DATE);
    
    // create list of sockets
    DEBUG("Creating list of sockets");
    Sockets sockets;
    socketsInit(&sockets);
    
    // open server socket
    /*TRACE("opening server socket");
    int server_socket = get_server_socket(settings.port);
    if (server_socket < 0) {
        ERROR("getting server socket");
        return EXIT_FAILURE;
    }

    server_new(sockets, server_socket, content);
    LOG("waiting for connections");

    // loop forever directing network traffic
    for (;;) {
        if (poll(sockets->pollfds, sockets->count, -1) < 0 ) {
            PANIC("when polling");
        }

        for (size_t i = 0; i < sockets->count; i++) {
            if (sockets->pollfds[i].revents) {
                sockets->listeners[i](sockets, i);
            }
        }
    }*/

    // tidy up, but we should never get here?
    DEBUG("Tidying up");
    //close(server_socket);
    //content_generators_free(content);
    socketsFree(&sockets);
    DEBUG("Tidy up complete");
    
    return EXIT_SUCCESS;
}