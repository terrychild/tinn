#include <stdlib.h>

#include "lib/net/echo.h"
#include "lib/mem/memory.h"
#include "lib/bytes.h"

typedef struct {
    ServerOutbound server;
} Connection;

static void echo(void* context, Slice data) {
    Connection* connection = context;
    hexDump(data);
    serverSend(connection->server, data);
    serverRead(connection->server, data.length);
}

static ServerInbound onConnect(__attribute__((unused)) void* context, ServerOutbound server) {
    Connection* connection = allocate(server.mem, sizeof(connection));
    connection->server = server;

    return (ServerInbound) {
        .onReceived = echo,
        .context = connection
    };
}

bool echoServer(ServerInterface server) {
    return serverAddListener(server, onConnect, NULL);
}