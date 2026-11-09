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
    Slice method;
    URL* target;
    Slice version;
    Slice host;
    Slice connection;
    time_t if_modified_since;
    U64 header_length;
    U64 content_length;
    Slice content;
} Request;

typedef struct {
    HttpStatusCode status_code;
    Array* headers;
    Slice content_type;
    U64 content_length;
    Slice content;
} Response;

typedef struct HttpConnection {
    ServerInterface server;
    Memory* exchange_mem;
    ConnectionStatus status;
    Request request;
    Response response;
} Connection;

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

void httpServerSend(HttpConnection* connection) {
    connection->status = SEND_HEADER;
    serverSend(connection->server, generateResponseHeader(connection));
}

// events
static void onReceived(void* context, Slice data) {
    Connection* connection = context;
    httpServerSendError(connection, HTTP_NOT_IMPLEMENTED);

    /*if (connection->status == RECEIVE_HEADER) {
        Slice header = sliceLeftStr(data, "\r\n\r\n");
        if (header.length > 0) {
        }
    } else if (connection->status == RECEIVE_CONTENT) {
        // TODO: read content
        httpServerSendError(connection, HTTP_NOT_IMPLEMENTED);
    }*/
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
        connection->request = (Request) {};
        connection->response = (Response) {
            .headers = arrayNew(connection->exchange_mem, sizeof(HttpHeader), 32)
        };

        //TODO: or close
    }
}

static void onDisconnected(void* context) {
    Connection* connection = context;
    memoryRelease(connection->exchange_mem);
}

static ServerClientInterface onConnect(__attribute__((unused)) void* context, ServerInterface server) {
    Connection* connection = allocate(server.mem, sizeof(connection));
    connection->server = server;
    connection->exchange_mem = memoryNew(MiB(100));
    connection->status = RECEIVE_HEADER;
    connection->response.headers = arrayNew(connection->exchange_mem, sizeof(HttpHeader), 32);

    return (ServerClientInterface) {
        .onReceived = onReceived,
        .onSent = onSent,
        .onDisconnected = onDisconnected,
        .context = connection
    };
}

Server* httpServer(Memory* mem, Polling* polling, const char* port) {
    return serverNew(mem, polling, port, onConnect, NULL);
}