#ifndef LIB_NET_HTTP_H
#define LIB_NET_HTTP_H

#define __USE_XOPEN

#include <time.h>

#include "lib/types.h"
#include "lib/sys/polling.h"
#include "lib/net/server.h"
#include "lib/net/url.h"

typedef enum {
    HTTP_OK = 200,
    HTTP_NO_CONTENT = 204,
    HTTP_NOT_MODIFIED = 304,
    HTTP_PERMANENT_REDIRECT = 308,
    HTTP_BAD_REQUEST = 400,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_VERSION_NOT_SUPPORTED = 505
} HttpStatusCode;

typedef enum {
    HTTP_RECEIVE_HEADER,
    HTTP_RECEIVE_CONTENT,
    HTTP_SEND_HEADER,
    HTTP_SEND_CONTENT,
} HttpServerConnectionStatus;

typedef struct {
    Slice name;
    Slice value;
} HttpHeader;

typedef struct {
    Slice method;
    URL target;
    Slice version;
    Slice host;
    Slice connection;
    time_t if_modified_since;
    Slice content;
} HttpServerRequest;

typedef struct {
    const char* version;
    HttpStatusCode status_code;
    const char* content_type;
    Array* headers;
    Slice content;
} HttpServerResponse;

typedef struct {
    ServerConnection* connection;
    Allocator* scope;
    HttpServerConnectionStatus status;
    HttpServerRequest* request;
    HttpServerResponse* response;
} HttpServerExchange;

Server* httpServer(Allocator* allocator, Polling* polling, const char* port);

#endif