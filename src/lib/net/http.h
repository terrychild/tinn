#ifndef LIB_NET_HTTP_H
#define LIB_NET_HTTP_H

#include "lib/types.h"
#include "lib/sys/polling.h"
#include "lib/net/server.h"

typedef struct {
    Slice name;
    Slice value;
} HttpHeader;

typedef struct {
    Array* headers;
    Buffer* content;
} HttpMessage;

typedef enum {
    RECEIVE_HEADER,
    RECEIVE_CONTENT,
} HttpServerConnectionStatus;

typedef struct {
    HttpServerConnectionStatus status;
    HttpMessage* request;
} HttpServerConnection;

Server* httpServer(Allocator* allocator, Polling* polling, const char* port);

#endif