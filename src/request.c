#include <sys/socket.h>
#include <string.h>

#include "request.h"
#include "utils.h"
#include "console.h"

Request* request_new() {
	Request* request = allocate(NULL, sizeof(*request));
	request->buf = buf_new(1024);
	request->target = NULL;
	request_reset(request);
	return request;	
}

void request_free(Request* request) {
	if (request!=NULL) {
		buf_free(request->buf);
		if (request->target != NULL) {
			uri_free(request->target);
		}
		free(request);
	}
}

Token default_header(char* value) {
	Token rv = {
		.start = value,
		.length = strlen(value)
	};
	return rv;
}

void request_reset(Request* request) {
	request->complete = false;

	buf_reset(request->buf);
	request->content_start = -1;

	request->method.length = 0;
	if (request->target != NULL) {
		uri_free(request->target);
		request->target = NULL;
	}
	request->version.length = 0;

	request->host = default_header("");
	request->connection = default_header("");
	request->if_modified_since = 0;
}

static int find_content(Buffer* buf) {
	for (int i=3; i<buf->length; i++) {
		if (buf->data[i-3]=='\r' && buf->data[i-2]=='\n' && buf->data[i-1]=='\r' && buf->data[i]=='\n') {
			return i+1;
		}
	}
	return -1;
}

ssize_t request_recv(Request* request, int socket) {
	int recvied = recv(socket, buf_write_ptr(request->buf), buf_write_max(request->buf), 0);
	if (recvied > 0) {
		TRACE("recived: %d", recvied);

		// update buffer
		buf_advance_write(request->buf, recvied);

		// check for content start
		if (request->content_start < 0) {
			request->content_start = find_content(request->buf);

			if (request->content_start < 0) {
				if (buf_write_max(request->buf) < 128) {
					buf_grow(request->buf);
				}
			} else {
				TRACE("header complete");

				// read header
				Scanner scanner = scanner_new(request->buf->data, request->content_start);

				// start line
				request->start_line = scan_token(&scanner, "\r\n");
				Scanner start_scanner = scanner_new(request->start_line.start, request->start_line.length);
				request->method = scan_token(&start_scanner, " ");
				request->target = uri_new(scan_token(&start_scanner, " "));
				request->version = scan_token(&start_scanner, "");

				TRACE_DETAIL("%.*s %s %.*s", request->method.length, request->method.start, request->target->path, request->version.length, request->version.start);

				// other headers
				Token line;
				while ((line = scan_token(&scanner, "\r\n")).length>0) {
					Scanner header_scanner = scanner_new(line.start, line.length);
					Token name = scan_token(&header_scanner, ": \t");
					Token value = scan_token(&header_scanner, "");

					TRACE_DETAIL("%.*s: %.*s", name.length, name.start, value.length, value.start);
					if (token_is(name, "Host")) {
						request->host = value;
					} else if (token_is(name, "Connection")) {
						request->connection = value;
					} else if (token_is(name, "If-Modified-Since")) {
						request->if_modified_since = from_imf_date(value.start, value.length);
					}
				}

				if (request->connection.length==0) {
					if (token_is(request->version, "HTTP/1.0")) {
						request->connection = default_header("close");
					} else {
						request->connection = default_header("keep-alive");
					}
				}

				// complete?
				// TODO: for post/patch/put there would be a content body to read....
				request->complete = true;
			}
		} else {
			// TODO: read content
		}		
	}
	return recvied;
}