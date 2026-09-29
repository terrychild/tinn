#include <string.h>
#include <time.h>

#include "lib/net/http.h"
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

static const char* validiateContentType(const char* content_type) {
	if (content_type != NULL && strlen(content_type) > 0) {
		if (content_type[0] == '.') {
			content_type += 1;
		}

		if (strcmp(content_type, "html")==0 || strcmp(content_type, "htm")==0) {
			return "text/html; charset=utf-8";
		} else if (strcmp(content_type, "css")==0) {
			return "text/css; charset=utf-8";
		} else if (strcmp(content_type, "js")==0) {
			return "text/javascript; charset=utf-8";
		} else if (strcmp(content_type, "jpeg")==0 || strcmp(content_type, "jpg")==0) {
			return "image/jpeg";
		} else if (strcmp(content_type, "png")==0) {
			return "image/png";
		} else if (strcmp(content_type, "gif")==0) {
			return "image/gif";
		} else if (strcmp(content_type, "bmp")==0) {
			return "image/bmp";
		} else if (strcmp(content_type, "svg")==0) {
			return "image/svg+xml";
		} else if (strcmp(content_type, "ico")==0) {
			return "image/vnd.microsoft.icon";
		} else if (strcmp(content_type, "mp3")==0) {
			return "audio/mpeg";
		}

        return content_type;
	}

	return "text/plain; charset=utf-8";
}

// message generation
static Slice generateResponseHeader(HttpServerConnectionContext* context) {
    Buffer* header = bufNew(context->connection->allocator, KB(4));

    // status line
    bufAppendFormat(header, "%s %d %s\r\n", context->response->version, context->response->status_code, status_text[context->response->status_code]);

    // date header
	bufAppendStr(header, "Date: ");
	toImfDate((char *)bufReadyWrite(header, IMF_DATE_LEN).start, IMF_DATE_LEN, time(NULL));
	bufConfirmWrite(header, IMF_DATE_LEN-1);
	bufAppendStr(header, "\r\n");

	// server header
	bufAppendStr(header, "Server: Tinn\r\n");

	// content headers
    if (context->response->content_type != NULL || context->response->content.length > 0) {
        bufAppendFormat(header, "Content-Type: %s\r\n", context->response->content_type);
        bufAppendFormat(header, "Content-Length: %ld\r\n", context->response->content.length);
    }

	// other headers
	/*for (size_t i=0; i<response->headers_count; i++) {
		buf_append_format(response->headers, "%s: %s\r\n", response->header_names[i], response->header_values[i]);
	}*/

	// close with empty line
	bufAppendStr(header, "\r\n");

    return bufAsSlice(header);
}

void httpServerSetStatus(HttpServerConnectionContext* context, HttpStatusCode status_code) {
    context->response->status_code = status_code;
}

void httpServerSetContent(HttpServerConnectionContext* context, char* content_type, Slice content) {
    context->response->content_type = validiateContentType(content_type);
    context->response->content = content;
}

void httpServerSend(HttpServerConnectionContext* context) {
    context->status = HTTP_SEND_HEADER;
    connectionSend(context->connection, generateResponseHeader(context));
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

void httpServerSendError(HttpServerConnectionContext* context, HttpStatusCode status_code) {
    Buffer* content = bufNew(context->connection->allocator, KB(4));
    bufAppendFormat(content, ERROR_TEMPLATE, status_code, status_text[status_code]);

    httpServerSetStatus(context, status_code);
    httpServerSetContent(context, "html", bufAsSlice(content));
    httpServerSend(context);
}

// events
static void onConnect(ServerConnection* connection) {
    HttpServerConnectionContext* context = allocate(connection->allocator, sizeof(HttpServerConnectionContext));
    context->connection = connection;
    context->status = HTTP_WAITING;
    context->request = NULL;
    context->response = NULL;

    connection->context = context;
}

static void onReceive(ServerConnection* connection, Slice data) {
    HttpServerConnectionContext* context = (HttpServerConnectionContext*)connection->context;

    if (context->status == HTTP_WAITING) {
        context->allocator = allocateChild(connection->allocator);
        context->request = allocate(connection->allocator, sizeof(HttpServerRequest));
        context->response = allocate(connection->allocator, sizeof(HttpServerResponse));
        context->response->version = "HTTP/1.1";
        //context->response.headers = arrayNew(connection->allocator, sizeof(HttpHeader), 32);

        context->status = HTTP_RECEIVE_HEADER;
    }

    if (context->status == HTTP_RECEIVE_HEADER) {
        Slice header = sliceLeftStr(data, "\r\n\r\n");
        if (header.length > 0) {
            Tokeniser lines = sliceTokeniserStr(header, "\r\n");

            // request line
            Slice request_line = nextToken(&lines);
            Tokeniser words = sliceTokeniserStr(request_line, " ");
            context->request->method = nextToken(&words);
            context->request->target = nextToken(&words);
            context->request->version = nextToken(&words);

            if (context->request->method.length == 0 || context->request->target.length == 0 || context->request->version.length == 0) {
                httpServerSendError(context, HTTP_BAD_REQUEST);
            } else {
                DEBUG("Request line: %.*s %.*s %.*s",
                    context->request->method.length, context->request->method.start,
                    context->request->target.length, context->request->target.start,
                    context->request->version.length, context->request->version.start
                );

                // temp response
                httpServerSetStatus(context, HTTP_OK);
                httpServerSetContent(context, "html", sliceFromStr("Hello world!"));
                httpServerSend(context);
            }
        }

    } else if (context->status == HTTP_RECEIVE_CONTENT) {
        // TODO: read content
        httpServerSendError(context, HTTP_NOT_IMPLEMENTED);
    }
}

static void onSent(ServerConnection* connection) {
    HttpServerConnectionContext* context = (HttpServerConnectionContext*)connection->context;

    if (context->status == HTTP_SEND_HEADER) {
        context->status = HTTP_SEND_CONTENT;
        connectionSend(connection, context->response->content);

    } else if (context->status == HTTP_SEND_CONTENT) {
        allocatorDebug(connection->server->allocator);
        deallocateChild(connection->allocator, context->allocator);
        allocatorDebug(connection->server->allocator);
        context->status = HTTP_WAITING;
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
