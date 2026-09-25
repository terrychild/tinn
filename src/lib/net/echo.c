#include <stdlib.h>

#include "lib/net/echo.h"
#include "lib/slice.h"
#include "lib/bytes.h"

static void echo(ServerConnection* connection, Slice data) {
    hexDump(data);
    connectionSend(connection, data);
}

Server* echoServer(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        server->onReceive = echo;
    }    
    return server;
}