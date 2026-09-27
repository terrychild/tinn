#include <time.h>

#include "lib/net/http.h"
#include "lib/net/mime.h"
#include "lib/macros.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/slice.h"
#include "lib/log.h"

#include "lib/bytes.h"

static const char* status_text[] = {
    [HTTP_OK] = "OK",
    [HTTP_MOVED_PERMANENTLY] = "Moved Permanently",
    [HTTP_NOT_MODIFIED] = "Not Modified",
    [HTTP_BAD_REQUEST] = "Bad Request",
    [HTTP_NOT_FOUND] = "Not Found",
    [HTTP_METHOD_NOT_ALLOWED] = "Method Not Allowed",
    [HTTP_INTERNAL_SERVER_ERROR] = "Internal Server Error",
    [HTTP_NOT_IMPLEMENTED] = "Not Implemented",
    [HTTP_VERSION_NOT_SUPPORTED] = "HTTP Version Not Supported"
};

static const size_t IMF_DATE_LEN = 30; // length of a date in Internet Messaging Format with null terminator
static char* toImfDate(char* buf, size_t max_len, time_t seconds) {
	strftime(buf, max_len, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&seconds));
	return buf;
}
/*static time_t fromImfDate(const char* date, size_t len) {
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	if (strptime(date, "%a, %d %b %Y %H:%M:%S GMT", &tm) == NULL) {
		ERROR("Invalid IMF date (%.*s)", len, date);
		return 0;
	}
	return mktime(&tm);
}*/

// message generation
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

void responseError(HttpServerResponse* response, HttpStatusCode status_code) {
    response->status_code = status_code;
    response->content_type = "html";
    bufReset(response->content);
	bufAppendFormat(response->content, ERROR_TEMPLATE, status_code, status_text[status_code]);
}

static Slice generateResponseHeader(HttpServerResponse* response) {
    bufReset(response->header);

    // status line
    bufAppendFormat(response->header, "%s %d %s\r\n", response->version, response->status_code, status_text[response->status_code]);

    // date header
	bufAppendStr(response->header, "Date: ");
	toImfDate((char *)bufReadyWrite(response->header, IMF_DATE_LEN).start, IMF_DATE_LEN, time(NULL));
	bufConfirmWrite(response->header, IMF_DATE_LEN-1);
	bufAppendStr(response->header, "\r\n");

	// server header
	bufAppendStr(response->header, "Server: Tinn\r\n");

	// content headers
    if (response->content_type != NULL || response->content->length > 0) {
        bufAppendFormat(response->header, "Content-Type: %s\r\n", mimeFromExt(response->content_type));
        bufAppendFormat(response->header, "Content-Length: %ld\r\n", response->content->length);
    }

	// other headers
	/*for (size_t i=0; i<response->headers_count; i++) {
		buf_append_format(response->headers, "%s: %s\r\n", response->header_names[i], response->header_values[i]);
	}*/

	// close with empty line
	bufAppendStr(response->header, "\r\n");

    return bufAsSlice(response->header);
}

// events
static void onConnect(ServerConnection* connection) {
    HttpServerConnection* context = allocate(connection->allocator, sizeof(HttpServerConnection));
    context->status = HTTP_RECEIVE_HEADER;

    context->request.headers = arrayNew(connection->allocator, sizeof(HttpHeader), 32, 0);

    context->response.version = "HTTP/1.1";
    context->response.content_type = NULL;
    context->response.headers = arrayNew(connection->allocator, sizeof(HttpHeader), 32, 0);
    context->response.header = bufNew(connection->allocator, KB(4));
    context->response.content = bufNew(connection->allocator, KB(4));

    connection->context = context;
}

static void onReceive(ServerConnection* connection, Slice data) {
    HttpServerConnection* context = (HttpServerConnection*)connection->context;
    if (context->status == HTTP_RECEIVE_HEADER) {
        Slice header = sliceLeftStr(data, "\r\n\r\n");
        if (header.length > 0) {
            Tokeniser lines = sliceTokeniserStr(header, "\r\n");

            // request line
            Slice request_line = nextToken(&lines);
            Tokeniser words = sliceTokeniserStr(request_line, " ");
            context->request.method = nextToken(&words);
            context->request.target = nextToken(&words);
            context->request.version = nextToken(&words);

            if (context->request.method.length == 0 || context->request.target.length == 0 || context->request.version.length == 0) {
                context->status = HTTP_SEND_HEADER;
                responseError(&context->response, HTTP_BAD_REQUEST);
                connectionSend(connection, generateResponseHeader(&context->response));
                return;
            }
            DEBUG("Request line: %.*s %.*s %.*s",
                context->request.method.length, context->request.method.start,
                context->request.target.length, context->request.target.start,
                context->request.version.length, context->request.version.start
            );

            // temp response
            responseError(&context->response, HTTP_NOT_IMPLEMENTED);
            //context->response.status_code = HTTP_NOT_IMPLEMENTED;
            //bufAppendStr(context->response.content, "Hello, World!");

            context->status = HTTP_SEND_HEADER;
            connectionSend(connection, generateResponseHeader(&context->response));
        }
    } else if (context->status == HTTP_RECEIVE_CONTENT) {
        // TODO: read content
    }
}

static void onSent(ServerConnection* connection) {
    HttpServerConnection* context = (HttpServerConnection*)connection->context;
    if (context->status == HTTP_SEND_HEADER) {
        context->status = HTTP_SEND_CONTENT;
        connectionSend(connection, bufAsSlice(context->response.content));
    } else if (context->status == HTTP_SEND_CONTENT) {
        context->status = HTTP_RECEIVE_HEADER;
        connectionSent(connection);
    }
}

Server* httpServer(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        server->onConnect = onConnect,
        server->onReceive = onReceive;
        server->onSent = onSent;
    }
    return server;
}
