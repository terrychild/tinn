#include "lib/net/http.h"
#include "lib/macros.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/slice.h"
#include "lib/log.h"

#include "lib/bytes.h"

static void onConnect(ServerConnection* connection) {
    HttpServerConnection* context = allocate(connection->allocator, sizeof(HttpServerConnection));
    context->status = RECEIVE_HEADER;
    context->request = allocate(connection->allocator, sizeof(HttpMessage));
    context->request->headers = arrayNew(connection->allocator, sizeof(HttpHeader), 32, 0);
    context->request->content = bufNew(connection->allocator, KB(4), 0);
    connection->context = context;
}

static void onReceive(ServerConnection* connection, Slice data) {
    DEBUG("http receive");
    HttpServerConnection* context = (HttpServerConnection*)connection->context;
    if (context->status == RECEIVE_HEADER) {
        Slice header = sliceLeftStr(data, "\r\n\r\n");
        if (header.length > 0) {
            DEBUG("header complete");
            context->status = RECEIVE_CONTENT;

            Tokeniser lines = sliceTokeniserStr(header, "\r\n");

            // request line
            Slice request_line = nextToken(&lines);
            Tokeniser words = sliceTokeniserStr(request_line, " ");

            Slice method = nextToken(&words);
            if (method.length == 0) {
                DEBUG("invalid method");
                return;
            }
            Slice target = nextToken(&words);
            if (target.length == 0) {
                DEBUG("invalid target");
                return;
            }
            Slice version = nextToken(&words);
            if (version.length == 0) {
                DEBUG("invalid version");
                return;
            }

            DEBUG("method: %.*s", method.length, method.start);
            DEBUG("target: %.*s", target.length, target.start);
            DEBUG("version: %.*s", version.length, version.start);

        }
    } else if (context->status == RECEIVE_CONTENT) {
        // TODO: read content
    }
}

Server* httpServer(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        server->onConnect = onConnect,
        server->onReceive = onReceive;
    }    
    return server;
}
