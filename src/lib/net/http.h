#ifndef LIB_NET_HTTP_H
#define LIB_NET_HTTP_H

#include "lib/sys/polling.h"
#include "lib/net/server.h"

typedef enum {
    HTTP_OK = 200,
    HTTP_NO_CONTENT = 204,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_NOT_MODIFIED = 304,
    HTTP_PERMANENT_REDIRECT = 308,
    HTTP_BAD_REQUEST = 400,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_VERSION_NOT_SUPPORTED = 505
} HttpStatusCode;

typedef struct {
    Slice name;
    Slice value;
} HttpHeader;

typedef struct HttpConnection HttpConnection;

Server* httpServer(Memory* mem, Polling* polling, const char* port);
void httpServerSetStatus(HttpConnection* connection, HttpStatusCode status_code);
void httpServerSetContentHeaders(HttpConnection* connection, Slice content_type, U64 content_length);
void httpServerSetContent(HttpConnection* connection, Slice content_type, Slice content);
void httpServerSendError(HttpConnection* connection, HttpStatusCode status_code);
void httpServerSend(HttpConnection* connection);

#endif