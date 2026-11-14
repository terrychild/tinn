#include <stdlib.h>
#include <string.h>
#define __USE_XOPEN // required for strptime
#include <time.h>

#include "lib/net/http.h"
#include "lib/net/url.h"
#include "lib/macros.h"
#include "lib/mem/memory.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"
#include "lib/bytes.h"
#include "lib/log.h"

static const char* status_text[] = {
    [HTTP_OK] = "OK",
    [HTTP_NO_CONTENT] = "No Content",
    [HTTP_MOVED_PERMANENTLY] = "Moved Permanently",
    [HTTP_NOT_MODIFIED] = "Not Modified",
    [HTTP_PERMANENT_REDIRECT] = "Permanent Redirect",
    [HTTP_BAD_REQUEST] = "Bad Request",
    [HTTP_NOT_FOUND] = "Not Found",
    [HTTP_METHOD_NOT_ALLOWED] = "Method Not Allowed",
    [HTTP_INTERNAL_SERVER_ERROR] = "Internal Server Error",
    [HTTP_NOT_IMPLEMENTED] = "Not Implemented",
    [HTTP_VERSION_NOT_SUPPORTED] = "HTTP Version Not Supported"
};

typedef enum {
    RECEIVE_HEADER,
    RECEIVE_CONTENT,
    SEND_HEADER,
    SEND_CONTENT,
} ConnectionStatus;

typedef struct {
    HttpStatusCode status_code;
    Array* headers;
    Slice content_type;
    U64 content_length;
    Slice content;
} Response;

typedef struct HttpConnection {
    ServerOutbound server;
    Memory* exchange_mem;
    Array* listeners;
    ConnectionStatus status;
    HttpServerRequest request;
    Response response;
} Connection;

typedef struct {
    HttpServerListenerFunc func;
    void* context;
} Listener;

struct HttpServer {
    Array* listeners;
};

// date formating
static const size_t IMF_DATE_LEN = 30; // length of a date in Internet Messaging Format with null terminator
static char* toImfDate(char* buf, size_t max_len, time_t seconds) {
    strftime(buf, max_len, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&seconds));
    return buf;
}
static time_t fromImfDate(const char* date, size_t len) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    if (strptime(date, "%a, %d %b %Y %H:%M:%S GMT", &tm) == NULL) {
        ERROR("Invalid IMF date (%.*s)", len, date);
        return 0;
    }
    return mktime(&tm);
}

// validators
static Slice validateContentType(Slice content_type) {
    if (content_type.length > 0) {
        if (content_type.start[0] == '.') {
            content_type.start += 1;
            content_type.length -= 1;
        }

        if (sliceIsStr(content_type, "html") || sliceIsStr(content_type, "htm")) {
            return sliceFromStr("text/html; charset=utf-8");
        } else if (sliceIsStr(content_type, "css")) {
            return sliceFromStr("text/css; charset=utf-8");
        } else if (sliceIsStr(content_type, "js")) {
            return sliceFromStr("text/javascript; charset=utf-8");
        } else if (sliceIsStr(content_type, "jpeg") || sliceIsStr(content_type, "jpg")) {
            return sliceFromStr("image/jpeg");
        } else if (sliceIsStr(content_type, "png")) {
            return sliceFromStr("image/png");
        } else if (sliceIsStr(content_type, "gif")) {
            return sliceFromStr("image/gif");
        } else if (sliceIsStr(content_type, "bmp")) {
            return sliceFromStr("image/bmp");
        } else if (sliceIsStr(content_type, "svg")) {
            return sliceFromStr("image/svg+xml");
        } else if (sliceIsStr(content_type, "ico")) {
            return sliceFromStr("image/vnd.microsoft.icon");
        } else if (sliceIsStr(content_type, "mp3")) {
            return sliceFromStr("audio/mpeg");
        }

        return content_type;
    }

    return sliceFromStr("text/plain; charset=utf-8");
}

// message generation
static Slice generateResponseHeader(HttpConnection* connection) {
    Buffer* header = bufNew(connection->exchange_mem, KiB(4));

    // default status code
    if (connection->response.status_code == 0) {
        connection->response.status_code = connection->response.content_length == 0 ? HTTP_NO_CONTENT : HTTP_OK;
    }

    // status line
    bufAppend(header, "HTTP/1.1 %d %s\r\n", connection->response.status_code, status_text[connection->response.status_code]);

    // date header
    bufAppend(header, "Date: ");
    toImfDate((char *)bufReadyWrite(header, IMF_DATE_LEN).start, IMF_DATE_LEN, time(NULL));
    bufConfirmWrite(header, IMF_DATE_LEN-1);
    bufAppend(header, "\r\n");

    // server header
    bufAppend(header, "Server: Tinn\r\n");

    // content headers
    if (connection->response.status_code != HTTP_NO_CONTENT && connection->response.status_code != HTTP_NOT_MODIFIED) {
        bufAppend(header, "Content-Length: %ld\r\n", connection->response.content_length);
        if (connection->response.content_type.length > 0) {
            bufAppend(header, "Content-Type: %.*s\r\n", connection->response.content_type.length, connection->response.content_type.start);
        }
    }

    // other headers
    for (size_t i=0; i<connection->response.headers->count; i++) {
        HttpHeader* kvp = (HttpHeader*)arrayGet(connection->response.headers, i);
        bufAppend(header, "%.*s: %.*s\r\n", kvp->name.length, kvp->name.start, kvp->value.length, kvp->value.start);
    }

    // close with empty line
    bufAppend(header, "\r\n");

    return bufAsSlice(header);
}

void httpServerSetStatus(HttpConnection* connection, HttpStatusCode status_code) {
    connection->response.status_code = status_code;
}

void httpServerSetContentHeaders(HttpConnection* connection, Slice content_type, U64 content_length) {
    connection->response.content_type = validateContentType(content_type);
    connection->response.content_length = content_length;
}
void httpServerSetContent(HttpConnection* connection, Slice content_type, Slice content) {
    connection->response.content_type = validateContentType(content_type);
    connection->response.content_length = content.length;
    connection->response.content = content;
}

void httpServerAddHeader(HttpConnection* connection, Slice name, Slice value) {
    HttpHeader* header = arrayAdd(connection->response.headers);
    header->name = name;
    header->value = value;
}
void httpServerAddDateHeader(HttpConnection* connection, Slice name, time_t seconds) {
    char* buf = allocate(connection->exchange_mem, IMF_DATE_LEN);
    httpServerAddHeader(connection, name, sliceFromStr(toImfDate(buf, IMF_DATE_LEN, seconds)));
}

void httpServerSend(HttpConnection* connection) {
    connection->status = SEND_HEADER;
    serverSend(connection->server, generateResponseHeader(connection));
}

void httpServerSendRedirect(HttpConnection* connection, Slice location) {
    if (sliceIsStr(connection->request.method, "GET")) {
        httpServerSetStatus(connection, HTTP_MOVED_PERMANENTLY);
    } else {
        httpServerSetStatus(connection, HTTP_PERMANENT_REDIRECT);
    }
    httpServerAddHeader(connection, sliceFromStr("Location"), location);
    httpServerSend(connection);
}

void httpServerSendNotModified(HttpConnection* connection) {
    httpServerSetStatus(connection, HTTP_NOT_MODIFIED);
    httpServerSend(connection);
}

#define ERROR_TEMPLATE \
    "<!DOCTYPE html>" \
    "<html lang=\"en\">" \
    "<head>" \
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0\">" \
    "<style>" \
    "body {color: #ffffff; background: #0000aa; font-family: monospace, monospace; margin: 10px;} " \
    ".title {color: #0000aa; background: #aaaaaa; padding-left: 1em; padding-right: 1em} " \
    "@media (width >= 660px) { .error {width: 640px; margin: 30vh auto 0;} } " \
    "@media (width < 660px) { .error {margin-top: 30px;} } " \
    "p {margin: 1em 0;} " \
    "</style>" \
    "<title>Error</title>" \
    "</head>" \
    "<body><div class=\"error\">" \
    "<p style=\"text-align: center;\"><span class=\"title\">Tinn</span></p>" \
    "<p>An error has occurred.  To continue:</p>" \
    "<p>Press F5 to refresh the page, it might work if you try again, or</p>" \
    "<p>Press ALT+F4 to quit your browser.  It's an extreme response but the error will go away, at least temporarily.</p>" \
    "<p>Error: %d : %s</p>" \
    "</div>" \
    "</body>" \
    "</html>"

void httpServerSendError(HttpConnection* connection, HttpStatusCode status_code) {
    Buffer* content = bufNew(connection->exchange_mem, KiB(4));
    bufAppend(content, ERROR_TEMPLATE, status_code, status_text[status_code]);

    httpServerSetStatus(connection, status_code);
    httpServerSetContent(connection, sliceFromStr("html"), bufAsSlice(content));
    httpServerSend(connection);
}

// events
static void onReceived(void* context, Slice data) {
    Connection* connection = context;

    if (connection->status == RECEIVE_HEADER) {
        Slice header = sliceLeftStr(data, "\r\n\r\n");
        if (header.length > 0) {
            connection->request.header_length = header.length + 4;

            Tokeniser lines = sliceTokeniserStr(header, "\r\n", false);

            // request line
            Slice request_line = nextToken(&lines);
            Tokeniser words = sliceTokeniserStr(request_line, " ", false);
            connection->request.method = nextToken(&words);
            connection->request.target = urlFromOrigin(connection->exchange_mem, nextToken(&words));
            connection->request.version = nextToken(&words);

            if (connection->request.method.length == 0 || !connection->request.target || connection->request.version.length == 0) {
                httpServerSendError(connection, HTTP_BAD_REQUEST);
            } else {
                DEBUG("Request: %.*s %.*s?%.*s",
                    connection->request.method.length, connection->request.method.start,
                    connection->request.target->path.length, connection->request.target->path.start,
                    connection->request.target->query.length, connection->request.target->query.start
                );

                // headers
                Slice line = nextToken(&lines);
                while (line.start) {
                    Slice name = sliceToLowerCase(connection->exchange_mem, sliceLeftStr(line, ":"));
                    Slice value = sliceTrim(sliceRightStr(line, ":"));

                    if (sliceIsStr(name, "host")) {
                        connection->request.host = value;
                    } else if (sliceIsStr(name, "connection")) {
                        connection->request.connection = value;
                    } else if (sliceIsStr(name, "if-modified-since")) {
                        connection->request.if_modified_since = fromImfDate((const char*)value.start, value.length);
                    }

                    line = nextToken(&lines);
                }

                if (connection->request.connection.length==0) {
                    if (sliceIsStr(connection->request.version, "HTTP/1.0")) {
                        connection->request.connection = sliceFromStr("close");
                    } else {
                        connection->request.connection = sliceFromStr("keep-alive");
                    }
                }

                // respond
                bool responded = false;
                for (U64 i=0; !responded && i<connection->listeners->count; i++) {
                    Listener* listener = arrayGet(connection->listeners, i);
                    responded = listener->func(connection->exchange_mem, connection, &connection->request, listener->context);
                }
                if (!responded) {
                    httpServerSendError(connection, HTTP_NOT_FOUND);
                }
            }
        }
    } else if (connection->status == RECEIVE_CONTENT) {
        // TODO: read content
        httpServerSendError(connection, HTTP_NOT_IMPLEMENTED);
    }
}

static void onSent(void* context) {
    Connection* connection = context;
    if (connection->status == SEND_HEADER && connection->response.content_length > 0 && !sliceIsStr(connection->request.method, "HEAD")) {
        connection->status = SEND_CONTENT;
        serverSend(connection->server, connection->response.content);
    } else {
        serverRead(connection->server, connection->request.header_length + connection->request.content_length);

        memoryReset(connection->exchange_mem);
        connection->status = RECEIVE_HEADER;
        connection->request = (HttpServerRequest) {};
        connection->response = (Response) {
            .headers = arrayNew(connection->exchange_mem, sizeof(HttpHeader), 32)
        };

        if (sliceIsStr(connection->request.connection, "close")) {
            serverDisconnect(connection->server);
        }
    }
}

static void onDisconnected(void* context) {
    Connection* connection = context;
    memoryRelease(connection->exchange_mem);
}

static ServerInbound onConnect(void* context, ServerOutbound server) {
    HttpServer* http_server = context;

    Connection* connection = allocate(server.mem, sizeof(connection));
    connection->server = server;
    connection->exchange_mem = memoryNew(MiB(100));
    connection->listeners = http_server->listeners;
    connection->status = RECEIVE_HEADER;
    connection->response.headers = arrayNew(connection->exchange_mem, sizeof(HttpHeader), 32);

    return (ServerInbound) {
        .onReceived = onReceived,
        .onSent = onSent,
        .onDisconnected = onDisconnected,
        .context = connection
    };
}

HttpServer* httpServer(Memory* mem, ServerInterface server) {
    HttpServer* http_server = allocate(mem, sizeof(HttpServer));
    if (!serverAddListener(server, onConnect, http_server)) {
        return NULL;
    }
    http_server->listeners = arrayNew(mem, sizeof(Listener), 1);
    return http_server;
}

void httpServerAddListener(HttpServer* server, HttpServerListenerFunc func, void* context) {
    Listener* listener = arrayAdd(server->listeners);
    listener->func = func;
    listener->context = context;
}