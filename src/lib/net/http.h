#ifndef LIB_NET_HTTP_H
#define LIB_NET_HTTP_H

#include <time.h>
#include "lib/net/server.h"
#include "lib/net/url.h"

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

typedef struct {
    Slice method;
    URL* target;
    Slice version;
    Slice host;
    Slice connection;
    time_t if_modified_since;
    U64 header_length;
    U64 content_length;
    Slice content;
} HttpServerRequest;

typedef struct HttpConnection HttpConnection;

typedef bool (*HttpServerListenerFunc)(Memory* exchange_mem, HttpConnection* connection, HttpServerRequest* request, void* context);

typedef struct HttpServer HttpServer;

HttpServer* httpServer(Memory* mem, ServerInterface server);
void httpServerAddListener(HttpServer* server, HttpServerListenerFunc func, void* context);

void httpServerSetStatus(HttpConnection* connection, HttpStatusCode status_code);
void httpServerSetContentHeaders(HttpConnection* connection, Slice content_type, U64 content_length);
void httpServerSetContent(HttpConnection* connection, Slice content_type, Slice content);
void httpServerAddHeader(HttpConnection* connection, Slice name, Slice value);
void httpServerAddDateHeader(HttpConnection* connection, Slice name, time_t seconds);

void httpServerSend(HttpConnection* connection);
void httpServerSendRedirect(HttpConnection* connection, Slice location);
void httpServerSendNotModified(HttpConnection* connection);
void httpServerSendError(HttpConnection* connection, HttpStatusCode status_code);

#endif