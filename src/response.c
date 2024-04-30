#include <string.h>
#include <sys/socket.h>

#include "response.h"
#include "utils.h"
#include "console.h"

#define RC_NONE 0
#define RC_INTERNAL 1
#define RC_EXTERNAL 2

Response* response_new() {
	Response* response = allocate(NULL, sizeof(*response));

	response->status_code = 500;

	response->headers_size = 8;
	response->headers_count = 0;
	response->header_names = allocate(NULL, sizeof(*response->header_names) * response->headers_size);
	response->header_values = allocate(NULL, sizeof(*response->header_values) * response->headers_size);

	response->content_source = RC_NONE;

	response->headers = buf_new(1024);
	response->stage = RESPONSE_PREP;

	return response;	
}

void response_reset(Response* response) {
	response->status_code = 500;

	for (size_t i=0; i<response->headers_count; i++) {
		free(response->header_names[i]);
		free(response->header_values[i]);
	}
	response->headers_count = 0;

	if (response->content_source == RC_INTERNAL) {
		buf_free(response->content);
	}
	response->content_source = RC_NONE;
	
	buf_reset(response->headers);
	response->stage = RESPONSE_PREP;
}

void response_free(Response* response) {
	if (response!=NULL) {
		for (size_t i=0; i<response->headers_count; i++) {
			free(response->header_names[i]);
			free(response->header_values[i]);
		}
		free(response->header_names);
		free(response->header_values);

		if (response->content_source == RC_INTERNAL) {
			buf_free(response->content);
		}

		buf_free(response->headers);

		free(response);
	}
}

void response_status(Response* response, int status_code) {
	response->status_code = status_code;
}

static char* status_text(int status) {
	switch (status) {
		case 200: return "Continue";
		case 301: return "Moved Permanently";
		case 304: return "Not Modified";
		case 400: return "Bad Request";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default:
			ERROR("Unknown status code %d", status);
			return "?";
	}
}

static char* copy_string(const char* value) {
	size_t len = strlen(value);
	char* new = allocate(NULL, len+1);
	return strcpy(new, value);
}

void response_header(Response* response, const char* name, const char* value) {
	for (size_t i=0; i<response->headers_count; i++) {
		if (strcmp(response->header_names[i], name)==0) {
			free(response->header_values[i]);
			response->header_values[i] = copy_string(value);
			return;
		}
	}

	if (response->headers_count == response->headers_size) {
		response->headers_size *= 2;

		response->header_names = allocate(response->header_names, sizeof(*response->header_names) * response->headers_size);
		response->header_values = allocate(response->header_values, sizeof(*response->header_values) * response->headers_size);
	}

	response->header_names[response->headers_count] = copy_string(name);
	response->header_values[response->headers_count] = copy_string(value);
	response->headers_count++;
}

void response_date(Response* response, const char* name, time_t date) {
	char buffer[IMF_DATE_LEN];
	to_imf_date(buffer, IMF_DATE_LEN, date);
	response_header(response, name, buffer);
}

void repsonse_no_content(Response* response) {
	if (response->content_source == RC_INTERNAL) {
		buf_free(response->content);
	}
	response->content_source = RC_NONE;
}

Buffer* response_content(Response* response, char* type) {
	if (response->content_source != RC_INTERNAL) {
		response->content = buf_new(1024);
	}
	response->content_source = RC_INTERNAL;
	response->type = content_type(type);
	return response->content;
}

void repsonse_link_content(Response* response, Buffer* buf, char* type) {
	if (response->content_source == RC_INTERNAL) {
		buf_free(response->content);
	}
	response->content_source = RC_EXTERNAL;
	response->content = buf;
	response->type = content_type(type);
}

static void build_headers(Response* response) {
	TRACE("build response headers");

	// status line
	buf_append_format(response->headers, "HTTP/1.1 %d %s\r\n", response->status_code, status_text(response->status_code));

	// date header
	buf_append_str(response->headers, ": ");
	to_imf_date(buf_reserve(response->headers, IMF_DATE_LEN), IMF_DATE_LEN, time(NULL));
	buf_advance_write(response->headers, -1);
	buf_append_str(response->headers, "\r\n");

	// server header
	buf_append_str(response->headers, "Server: Tinn\r\n");

	// content headers
	if (response->content_source != RC_NONE) {
		buf_append_format(response->headers, "Content-Type: %s\r\n", response->type);
		buf_append_format(response->headers, "Content-Length: %ld\r\n", response->content->length);
	}

	// other headers
	for (size_t i=0; i<response->headers_count; i++) {
		buf_append_format(response->headers, "%s: %s\r\n", response->header_names[i], response->header_values[i]);
	}

	// close with empty line
	buf_append_str(response->headers, "\r\n");

	response->stage++;
}

ssize_t response_send(Response* response, int socket) {
	if (response->stage == RESPONSE_PREP) {
		build_headers(response);
	}

	if (response->stage == RC_NONE) { // just in case?
		WARN("trying to send a request that is finished");
		return 0;
	}

	Buffer* buf = (response->stage == RESPONSE_HEADERS) ? response->headers : response->content;
	
	size_t len = buf_read_max(buf);
	ssize_t sent = send(socket, buf_read_ptr(buf), len, MSG_DONTWAIT);
	if (sent >= 0) {
		TRACE("sent %d: %ld/%ld", response->stage, sent, len);
		if ((size_t)sent < len) {
			buf_advance_read(buf, sent);
		} else {
			response->stage++;
			if (response->stage == RESPONSE_CONTENT && response->content_source == RC_NONE) {
				response->stage++;
			}
		}
	}
	return sent;	
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

void response_error(Response* response, int status_code) {
	response_status(response, status_code);
	buf_append_format(response_content(response, "html"), ERROR_TEMPLATE, status_code, status_text(status_code));
}

void response_redirect(Response* response, char* location) {
	response_status(response, 301);
	response_header(response, "Location", location);
}

#undef RC_NONE
#undef RC_INTERNAL
#undef RC_EXTERNAL