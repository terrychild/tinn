#ifndef LIB_NET_HTTP_H
#define LIB_NET_HTTP_H

#include "lib/types.h"
#include "lib/sys/polling.h"
#include "lib/net/server.h"

typedef enum {
    HTTP_OK = 200,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_NOT_MODIFIED = 304,
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
    Slice target;
    Slice version;
    Array* headers;
    Slice content;
} HttpServerRequest;

typedef struct {
    char* version;
    HttpStatusCode status_code;
    char* content_type;
    Array* headers;
    Buffer* header;
    Buffer* content;
} HttpServerResponse;

typedef struct {
    HttpServerConnectionStatus status;
    HttpServerRequest request;
    HttpServerResponse response;
} HttpServerConnection;

Server* httpServer(Allocator* allocator, Polling* polling, const char* port);

#endif