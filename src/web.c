#include <stdlib.h>

#include "web.h"
#include "lib/log.h"
#include "lib/slice.h"
#include "lib/bytes.h"

void echo(ServerConnection* connection, Slice data) {
    hexDump(data);
    if (sliceCmpStr(data, "quit\r\n") == 0) {
        serverClose(connection->server);
    } else {
        connectionSend(connection, data);
    }
}

Server* startWebServer(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        server->onReceive = echo;
    }    
    return server;
}