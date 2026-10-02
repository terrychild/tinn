#include <string.h>

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
    [HTTP_NO_CONTENT] = "No Content",
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

static const char* validateContentType(const char* content_type) {
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
    if (exchange->response->status_code != HTTP_NO_CONTENT) {
        bufAppendFormat(header, "Content-Length: %ld\r\n", exchange->response->content.length);
        if (exchange->response->content_type) {
            bufAppendFormat(header, "Content-Type: %s\r\n", exchange->response->content_type);
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

void httpServerSetContent(HttpServerExchange* exchange, char* content_type, Slice content) {
    exchange->response->content_type = validateContentType(content_type);
    exchange->response->content = content;
}

void httpServerAddHeader(HttpServerExchange* exchange, Slice name, Slice value) {
    HttpHeader* header = arrayAdd(exchange->response->headers);
    header->name = name;
    header->value = value;
}

void httpServerSend(HttpServerExchange* exchange) {
    exchange->status = HTTP_SEND_HEADER;
    connectionSend(exchange->connection, generateResponseHeader(exchange));
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
    httpServerSetContent(exchange, "html", bufAsSlice(content));
    httpServerSend(exchange);
}

void httpServerRedirect(HttpServerExchange* exchange, const char* location) {
    httpServerSetStatus(exchange, HTTP_PERMANENT_REDIRECT);
    httpServerAddHeader(exchange, sliceFromStr("Location"), sliceFromStr(location));
    httpServerSend(exchange);
}

// events
static HttpServerExchange* getExchange(ServerConnection* connection) {
    if (connection->context != NULL) {
        return (HttpServerExchange*)connection->context;
    }

    HttpServerExchange* exchange = allocate(connection->exchange_scope, sizeof(ServerConnection));
    exchange->connection = connection;
    exchange->scope = connection->exchange_scope;
    exchange->status = HTTP_RECEIVE_HEADER;
    exchange->request = allocate(exchange->scope, sizeof(HttpServerRequest));
    exchange->response = allocate(exchange->scope, sizeof(HttpServerResponse));
    exchange->response->version = "HTTP/1.1";
    exchange->response->headers = arrayNew(exchange->scope, sizeof(HttpHeader), 32);

    connection->context = exchange;
    return exchange;
}

static void onReceive(ServerConnection* connection, Slice data) {
    HttpServerExchange* exchange = getExchange(connection);

    if (exchange->status == HTTP_RECEIVE_HEADER) {
        Slice header = sliceLeftStr(data, "\r\n\r\n");
        if (header.length > 0) {
            Tokeniser lines = sliceTokeniserStr(header, "\r\n");

            // request line
            Slice request_line = nextToken(&lines);
            Tokeniser words = sliceTokeniserStr(request_line, " ");
            exchange->request->method = nextToken(&words);
            exchange->request->target = nextToken(&words);
            exchange->request->version = nextToken(&words);

            if (exchange->request->method.length == 0 || exchange->request->target.length == 0 || exchange->request->version.length == 0) {
                httpServerSendError(exchange, HTTP_BAD_REQUEST);
            } else {
                DEBUG("Request line: %.*s %.*s %.*s",
                    exchange->request->method.length, exchange->request->method.start,
                    exchange->request->target.length, exchange->request->target.start,
                    exchange->request->version.length, exchange->request->version.start
                );

                // headers
                Slice line = nextToken(&lines);
                while (line.length > 0) {
                    Slice name = sliceLeftStr(line, ":");
                    Slice value = sliceTrim(sliceRightStr(line, ":"));
                    //DEBUG("%.*s: %.*s", name.length, name.start, value.length, value.start);

                    if (sliceIsStr(name, "Host")) {
						exchange->request->host = value;
					} else if (sliceIsStr(name, "Connection")) {
						exchange->request->connection = value;
					} else if (sliceIsStr(name, "If-Modified-Since")) {
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

                DEBUG("Connection: %.*s", exchange->request->connection.length, exchange->request->connection.start);

                // temp response
                if (sliceIsStr(exchange->request->target, "/")) {
                    httpServerRedirect(exchange, "/index.html");
                } else {
                    httpServerSetStatus(exchange, HTTP_OK);
                    httpServerSetContent(exchange, "html", sliceFromStr("<html><body><h1>Hello world!</h1></body></html>"));
                    httpServerSend(exchange);
                }
            }
        }

    } else if (exchange->status == HTTP_RECEIVE_CONTENT) {
        // TODO: read content
        httpServerSendError(exchange, HTTP_NOT_IMPLEMENTED);
    }
}

static void onSent(ServerConnection* connection, bool* close) {
    if (connection->context != NULL) {
        HttpServerExchange* exchange = (HttpServerExchange*)connection->context;

        if (exchange->status == HTTP_SEND_HEADER) {
            exchange->status = HTTP_SEND_CONTENT;
            connectionSend(connection, exchange->response->content);

        } else if (exchange->status == HTTP_SEND_CONTENT) {
            if (sliceIsStr(exchange->request->connection, "close")) {
                *close = true;
            } else {
                connectionSent(connection, close);
                connection->context = NULL;
            }
            allocatorDebug(connection->server->scope);
        }
    }
}

Server* httpServer(Allocator* allocator, Polling* polling, const char* port) {
    Server* server = serverNew(allocator, polling, port);
    if (server) {
        server->onReceive = onReceive;
        server->onSent = onSent;
    }
    return server;
}
