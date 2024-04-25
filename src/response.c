#include <string.h>

#include "response.h"
#include "utils.h"
#include "console.h"

Response* response_new() {
	Response* response = allocate(NULL, sizeof(*response));

	response->headers_size = 8;
	response->headers_count = 0;
	response->header_names = allocate(NULL, sizeof(*response->header_names) * response->headers_size);
	response->header_values = allocate(NULL, sizeof(*response->header_values) * response->headers_size);

	response->content = buf_new(1024);

	response->buf = buf_new(1024);

	response_reset(response);

	return response;	
}

void response_free(Response* response) {
	if (response!=NULL) {
		for (size_t i=0; i<response->headers_count; i++) {
			free(response->header_names[i]);
			free(response->header_values[i]);
		}
		free(response->header_names);
		free(response->header_values);

		buf_free(response->content);

		buf_free(response->buf);

		free(response);
	}
}

void response_reset(Response* response) {
	TRACE("reset response");
	response->status_code = 500;

	for (size_t i=0; i<response->headers_count; i++) {
		free(response->header_names[i]);
		free(response->header_values[i]);
	}
	response->headers_count = 0;

	response->type = content_type(NULL);
	buf_reset(response->content);
	
	response->built = false;
	buf_reset(response->buf);
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

Buffer* response_content(Response* response, char* type) {
	response->type = content_type(type);
	return response->content;
}

Buffer* response_buf(Response* response) {
	if (!response->built) {
		TRACE("build response");

		// status line
		buf_append_format(response->buf, "HTTP/1.1 %d %s\r\n", response->status_code, status_text(response->status_code));

		// date header
		buf_append_str(response->buf, ": ");
		to_imf_date(buf_reserve(response->buf, IMF_DATE_LEN), IMF_DATE_LEN, time(NULL));
		buf_advance_write(response->buf, -1);
		buf_append_str(response->buf, "\r\n");

		// standard headers
		buf_append_str(response->buf, "Server: Tinn\r\n");
		buf_append_format(response->buf, "Content-Type: %s\r\n", response->type);
		buf_append_format(response->buf, "Content-Length: %ld\r\n", response->content->length);

		// other headers
		for (size_t i=0; i<response->headers_count; i++) {
			buf_append_format(response->buf, "%s: %s\r\n", response->header_names[i], response->header_values[i]);
		}

		// close header
		buf_append_str(response->buf, "\r\n");

		// content
		buf_append_buf(response->buf, response->content);
	}
	return response->buf;
}

void response_simple_status(Response* response, int status_code, char *description) {
	response_reset(response);
	response_status(response, status_code);
	buf_append_format(response_content(response, "html"), "<html><body><h1>%d - %s</h1><p>%s</p></body></html>", status_code, status_text(status_code), description);
}

/*
static void prep_redirect(ClientState* state, char* location) {
	start_headers(state->out, "301", "Moved Permanently");
	buf_append_format(state->out, "Location: %s\r\n", location);
	end_headers(state->out);
}



*/
