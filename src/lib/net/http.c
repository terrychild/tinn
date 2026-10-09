#include <string.h>

#include "lib/net/http.h"
#include "lib/macros.h"
#include "lib/mem/allocator.h"
#include "lib/mem/array.h"
#include "lib/mem/buffer.h"
#include "lib/mem/slice.h"
#include "lib/log.h"

#include "lib/bytes.h"

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
static Slice generateResponseHeader(HttpServerExchange* exchange) {
    Buffer* header = bufNew(exchange->scope, KB(4));

    // default status code
    if (exchange->response->status_code == 0) {
        exchange->response->status_code = exchange->response->content.length == 0 ? HTTP_NO_CONTENT : HTTP_OK;
    }

    // status line
    bufAppendFormat(header, "%s %d %s\r\n", exchange->response->version, exchange->response->status_code, status_text[exchange->response->status_code]);

    // date header
	bufAppendStr(header, "Date: ");
	toImfDate((char *)bufReadyWrite(header, IMF_DATE_LEN).start, IMF_DATE_LEN, time(NULL));
	bufConfirmWrite(header, IMF_DATE_LEN-1);
	bufAppendStr(header, "\r\n");

	// server header
	bufAppendStr(header, "Server: Tinn\r\n");

	// content headers
    if (exchange->response->status_code != HTTP_NO_CONTENT && exchange->response->status_code != HTTP_NOT_MODIFIED) {
        bufAppendFormat(header, "Content-Length: %ld\r\n", exchange->response->content.length);
        if (exchange->response->content_type.length > 0) {
            bufAppendFormat(header, "Content-Type: %.*s\r\n", exchange->response->content_type.length, exchange->response->content_type.start);
        }
    }

	// other headers
	for (size_t i=0; i<exchange->response->headers->count; i++) {
        HttpHeader* kvp = (HttpHeader*)arrayGet(exchange->response->headers, i);
		bufAppendFormat(header, "%.*s: %.*s\r\n", kvp->name.length, kvp->name.start, kvp->value.length, kvp->value.start);
	}

	// close with empty line
	bufAppendStr(header, "\r\n");

    return bufAsSlice(header);
}

void httpServerSetStatus(HttpServerExchange* exchange, HttpStatusCode status_code) {
    exchange->response->status_code = status_code;
}

void httpServerSetContentType(HttpServerExchange* exchange, Slice content_type) {
    exchange->response->content_type = validateContentType(content_type);
}
void httpServerSetContent(HttpServerExchange* exchange, Slice content_type, Slice content) {
    exchange->response->content_type = validateContentType(content_type);
    exchange->response->content = content;
}

void httpServerAddHeader(HttpServerExchange* exchange, Slice name, Slice value) {
    HttpHeader* header = arrayAdd(exchange->response->headers);
    header->name = name;
    header->value = value;
}
void httpServerAddDateHeader(HttpServerExchange* exchange, Slice name, time_t seconds) {
    char* buf = allocate(exchange->scope, IMF_DATE_LEN);
    httpServerAddHeader(exchange, name, sliceFromStr(toImfDate(buf, IMF_DATE_LEN, seconds)));
}

void httpServerSend(HttpServerExchange* exchange) {
    exchange->status = HTTP_SEND_HEADER;
    connectionSend(exchange->connection, generateResponseHeader(exchange));
}

void httpServerSendRedirect(HttpServerExchange* exchange, Slice location) {
    if (sliceIsStr(exchange->request->method, "GET")) {
        httpServerSetStatus(exchange, HTTP_MOVED_PERMANENTLY);
    } else {
        httpServerSetStatus(exchange, HTTP_PERMANENT_REDIRECT);
    }
    httpServerAddHeader(exchange, sliceFromStr("Location"), location);
    httpServerSend(exchange);
}

void httpServerSendNotModified(HttpServerExchange* exchange) {
    httpServerSetStatus(exchange, HTTP_NOT_MODIFIED);
    httpServerSend(exchange);
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

void httpServerSendError(HttpServerExchange* exchange, HttpStatusCode status_code) {
    Buffer* content = bufNew(exchange->scope, KB(4));
    bufAppendFormat(content, ERROR_TEMPLATE, status_code, status_text[status_code]);

    httpServerSetStatus(exchange, status_code);
    httpServerSetContent(exchange, sliceFromStr("html"), bufAsSlice(content));
    httpServerSend(exchange);
}

// events
static void onConnect(ServerConnection* connection) {
    HttpServer* http_server = (HttpServer*)connection->context;
    HttpServerConnection* http_connection = allocate(connection->scope, sizeof(HttpServerConnection));
    http_connection->server = http_server;
    connection->context = http_connection;
}

static void onReceive(ServerConnection* connection, Slice data) {
    HttpServerConnection* http_connection = (HttpServerConnection*)connection->context;
    HttpServerExchange* exchange = http_connection->exchange;
    if (!exchange) {
        exchange = allocate(connection->exchange_scope, sizeof(ServerConnection));
        exchange->connection = connection;
        exchange->scope = connection->exchange_scope;
        exchange->status = HTTP_RECEIVE_HEADER;
        exchange->request = allocate(exchange->scope, sizeof(HttpServerRequest));
        exchange->response = allocate(exchange->scope, sizeof(HttpServerResponse));
        exchange->response->version = "HTTP/1.1";
        exchange->response->headers = arrayNew(exchange->scope, sizeof(HttpHeader), 32);

        http_connection->exchange = exchange;
    }

    if (exchange->status == HTTP_RECEIVE_HEADER) {
        Slice header = sliceLeftStr(data, "\r\n\r\n");
        if (header.length > 0) {
            Tokeniser lines = sliceTokeniserStr(header, "\r\n", false);

            // request line
            Slice request_line = nextToken(&lines);
            Tokeniser words = sliceTokeniserStr(request_line, " ", false);
            exchange->request->method = nextToken(&words);
            exchange->request->target = urlFromOrigin(connection->exchange_scope, nextToken(&words));
            exchange->request->version = nextToken(&words);

            if (exchange->request->method.length == 0 || !exchange->request->target || exchange->request->version.length == 0) {
                httpServerSendError(exchange, HTTP_BAD_REQUEST);
            } else {
                DEBUG("Request: %.*s %.*s?%.*s",
                    exchange->request->method.length, exchange->request->method.start,
                    exchange->request->target->path.length, exchange->request->target->path.start,
                    exchange->request->target->query.length, exchange->request->target->query.start
                );
                /*for (U64 i=0; i<exchange->request->target->path_segments->count; i++) {
                    Slice* segment = (Slice*)arrayGet(exchange->request->target->path_segments, i);
                    DEBUG("Path segment (%lu): %.*s", segment->length, segment->length, segment->start);
                }*/

                // headers
                Slice line = nextToken(&lines);
                while (line.start) {
                    Slice name = sliceToLowerCase(sliceLeftStr(line, ":"));
                    Slice value = sliceTrim(sliceRightStr(line, ":"));
                    //DEBUG("%.*s: %.*s", name.length, name.start, value.length, value.start);

                    if (sliceIsStr(name, "host")) {
						exchange->request->host = value;
					} else if (sliceIsStr(name, "connection")) {
						exchange->request->connection = value;
					} else if (sliceIsStr(name, "if-modified-since")) {
						exchange->request->if_modified_since = fromImfDate((const char*)value.start, value.length);
					}

                    line = nextToken(&lines);
                }

                if (exchange->request->connection.length==0) {
					if (sliceIsStr(exchange->request->version, "HTTP/1.0")) {
						exchange->request->connection = sliceFromStr("close");
					} else {
						exchange->request->connection = sliceFromStr("keep-alive");
					}
				}

                // respond
                if (http_connection->server->onRequest) {
                    http_connection->server->onRequest(exchange, http_connection->server->context);
                } else {
                    httpServerSetStatus(exchange, HTTP_NO_CONTENT);
                    httpServerSend(exchange);
                }
            }
        }

    } else if (exchange->status == HTTP_RECEIVE_CONTENT) {
        // TODO: read content
        httpServerSendError(exchange, HTTP_NOT_IMPLEMENTED);
    }
}

static void onSent(ServerConnection* connection, bool* remove) {
    HttpServerConnection* http_connection = (HttpServerConnection*)connection->context;
    HttpServerExchange* exchange = http_connection->exchange;

    if (exchange) {
        if (exchange->status == HTTP_SEND_HEADER && exchange->response->content.length > 0) {
            exchange->status = HTTP_SEND_CONTENT;
            connectionSend(connection, exchange->response->content);

        } else {
            if (sliceIsStr(exchange->request->connection, "close")) {
                *remove = true;
            } else {
                connectionSent(connection, remove);
                http_connection->exchange = NULL;
            }
        }
    }
}

HttpServer* httpServer(Allocator* allocator, Polling* polling, const char* port) {
    HttpServer* http_server = NULL;
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        http_server = allocate(allocator, sizeof(HttpServer));

        server->onConnect = onConnect;
        server->onReceive = onReceive;
        server->onSent = onSent;
        server->context = http_server;
    }
    return http_server;
}
