#include <stdlib.h>

#include "lib/net/echo.h"
#include "lib/mem/memory.h"
#include "lib/bytes.h"

typedef struct {
    ServerInterface server;
} Connection;

static void echo(void* context, Slice data) {
    Connection* connection = context;
    hexDump(data);
    serverSend(connection->server, data);
    serverRead(connection->server, data.length);
}

static ServerClientInterface onConnect(__attribute__((unused)) void* context, ServerInterface server) {
    Connection* connection = allocate(server.mem, sizeof(connection));
    connection->server = server;

    return (ServerClientInterface) {
        .onReceived = echo,
        .context = connection
    };
}

Server* echoServer(Memory* mem, Polling* polling, const char* port) {
    return serverNew(mem, polling, port, onConnect, NULL);
}